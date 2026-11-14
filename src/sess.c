/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 xtaci
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "sess.h"
#include "fec.h"
#include "ikcp.h"
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
typedef SOCKET kcp_socket;
typedef int kcp_socklen;
#define KCP_INVALID_SOCKET INVALID_SOCKET
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
typedef int kcp_socket;
typedef socklen_t kcp_socklen;
#define KCP_INVALID_SOCKET (-1)
#endif

#ifdef _WIN32
/* Translate Winsock errors into the public errno-based error contract. */
static int socket_error(void) {
    switch (WSAGetLastError()) {
    case WSAEINTR: return EINTR;
    case WSAEWOULDBLOCK: return EAGAIN;
    case WSAEMSGSIZE: return EMSGSIZE;
    case WSAECONNREFUSED: return ECONNREFUSED;
    case WSAEINVAL: return EINVAL;
    default: return EIO;
    }
}
/* Pair each Windows session with one Winsock reference. */
static int socket_init(void) {
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data)) { errno = EIO; return -1; }
    return 0;
}
/* Close the socket and release its Winsock reference. */
static void socket_close(kcp_socket fd) { closesocket(fd); WSACleanup(); }
/* Make UDP receives nonblocking. */
static int socket_nonblocking(kcp_socket fd) {
    u_long enabled = 1;
    if (ioctlsocket(fd, FIONBIO, &enabled)) { errno = socket_error(); return -1; }
    return 0;
}
#else
/* POSIX socket calls already report failures through errno. */
static int socket_error(void) { return errno; }
/* POSIX sockets require no per-session initialization. */
static int socket_init(void) { return 0; }
/* Close a POSIX socket. */
static void socket_close(kcp_socket fd) { close(fd); }
/* Make UDP receives nonblocking. */
static int socket_nonblocking(kcp_socket fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return flags < 0 ? -1 : fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
#endif

/* State owned by one connected UDP/KCP session. */
struct kcp_session {
    kcp_socket fd;                   /* Connected nonblocking UDP socket. */
    int output_error;                /* First send failure during this update. */
    ikcpcb *kcp;                     /* KCP protocol state. */
    fec *fec, *rx_fec;              /* Send codec and possibly separate receive codec. */
    kcp_crypt *crypt;                /* Borrowed packet cipher, or NULL. */
    uint8_t input[FEC_MAX_PACKET];   /* Received datagram buffer. */
    uint8_t plain[FEC_MAX_PACKET];   /* Decrypted datagram buffer. */
    char *stream;                    /* Buffered unread portion of a KCP message. */
    size_t stream_size;              /* Number of unread bytes in stream. */
};

/* Encrypt a FEC or raw KCP packet and send it as one UDP datagram. */
static int send_packet(void *user, const uint8_t *data, size_t len) {
    kcp_session *s = (kcp_session *)user;
    const uint8_t *packet = data;
    uint8_t encrypted[FEC_MAX_PACKET];
#ifdef KCP_ENABLE_CRYPTO
    if (s->crypt) {
        size_t encrypted_len;
        if (kcp_crypt_seal(s->crypt, data, len, encrypted, sizeof(encrypted), &encrypted_len)) {
            s->output_error = errno; return -1;
        }
        packet = encrypted; len = encrypted_len;
    }
#else
    (void)encrypted;
#endif
    {
        int sent = (int)send(s->fd, (const char *)packet, (int)len, 0);
        if (sent != (int)len) {
            s->output_error = sent < 0 ? socket_error() : EIO; return -1;
        }
    }
    return 0;
}

/* Convert KCP output to FEC packets when sending FEC is enabled. */
static int kcp_output(const char *buf, int len, ikcpcb *kcp, void *user) {
    kcp_session *s = (kcp_session *)user;
    int result;
    (void)kcp;
    if (s->fec) result = fec_encode(s->fec, (const uint8_t *)buf, (size_t)len, send_packet, s);
    else result = send_packet(s, (const uint8_t *)buf, (size_t)len);
    if (result && !s->output_error) s->output_error = errno ? errno : EIO;
    return result;
}

/* Feed an original or recovered KCP packet into the protocol state. */
static int input_kcp(void *user, const uint8_t *data, size_t len) {
    kcp_session *s = (kcp_session *)user;
    if (len <= LONG_MAX) ikcp_input(s->kcp, (const char *)data, (long)len);
    return 0;
}

/* Create a single connected UDP socket and its KCP/FEC state. */
kcp_session *kcp_session_dial(const char *ip, uint16_t port, size_t data_shards,
                              size_t parity_shards, kcp_crypt *crypt) {
    struct sockaddr_storage addr;
    kcp_socklen addrlen;
    kcp_socket fd;
    int family;
    kcp_session *s;
    if (!ip || data_shards > 255 || parity_shards > 255 ||
        (data_shards && parity_shards && data_shards + parity_shards > 256)) {
        errno = EINVAL; return NULL;
    }
#ifndef KCP_ENABLE_CRYPTO
    if (crypt) { errno = ENOTSUP; return NULL; }
#endif
    memset(&addr, 0, sizeof(addr));
    if (inet_pton(AF_INET, ip, &((struct sockaddr_in *)&addr)->sin_addr) == 1) {
        struct sockaddr_in *v4 = (struct sockaddr_in *)&addr;
        family = AF_INET; v4->sin_family = AF_INET; v4->sin_port = htons(port);
        addrlen = sizeof(*v4);
    } else if (inet_pton(AF_INET6, ip, &((struct sockaddr_in6 *)&addr)->sin6_addr) == 1) {
        struct sockaddr_in6 *v6 = (struct sockaddr_in6 *)&addr;
        family = AF_INET6; v6->sin6_family = AF_INET6; v6->sin6_port = htons(port);
        addrlen = sizeof(*v6);
    } else { errno = EINVAL; return NULL; }
    /* Each successful socket initialization is paired with socket_close. */
    if (socket_init()) return NULL;
    fd = socket(family, SOCK_DGRAM, 0);
    if (fd == KCP_INVALID_SOCKET) {
        int err = socket_error();
#ifdef _WIN32
        WSACleanup();
#endif
        errno = err; return NULL;
    }
    if (connect(fd, (struct sockaddr *)&addr, addrlen)) {
        int err = socket_error(); socket_close(fd); errno = err; return NULL;
    }
    if (socket_nonblocking(fd)) {
        int err = errno; socket_close(fd); errno = err; return NULL;
    }
    s = (kcp_session *)calloc(1, sizeof(*s));
    if (!s) { int err = errno; socket_close(fd); errno = err; return NULL; }
    s->fd = fd; s->crypt = crypt;
    s->kcp = ikcp_create((IUINT32)rand(), s);
    if (!s->kcp) { errno = ENOMEM; goto fail; }
    ikcp_setoutput(s->kcp, kcp_output);
    if (data_shards && parity_shards) {
        s->fec = fec_create((int)data_shards, (int)parity_shards);
        if (!s->fec) goto fail;
        s->rx_fec = s->fec;
    }
#ifdef KCP_ENABLE_CRYPTO
    if (crypt) {
        size_t overhead = kcp_crypt_overhead(crypt) + (s->fec ? FEC_DATA_HEADER_SIZE : 0);
        size_t mtu;
        if (overhead >= 1472) { errno = EMSGSIZE; goto fail; }
        mtu = 1472 - overhead;
        if (mtu < s->kcp->mtu && kcp_session_set_mtu(s, (int)mtu)) goto fail;
    }
#endif
    return s;
fail:
    { int err = errno; kcp_session_destroy(s); errno = err; }
    return NULL;
}

/* Release all owned session resources and leave the borrowed cipher intact. */
void kcp_session_destroy(kcp_session *s) {
    if (!s) return;
    socket_close(s->fd);
    if (s->kcp) ikcp_release(s->kcp);
    if (s->rx_fec != s->fec) fec_destroy(s->rx_fec);
    fec_destroy(s->fec); free(s->stream); free(s);
}

/* Drain available datagrams, then flush pending KCP output. */
int kcp_session_update(kcp_session *s, uint32_t now) {
    if (!s) { errno = EINVAL; return -1; }
    s->output_error = 0;
    for (;;) {
        int n;
        const uint8_t *data = s->input;
        size_t len;
#ifdef _WIN32
        n = recv(s->fd, (char *)s->input, (int)sizeof(s->input), 0);
        if (n < 0) {
            errno = socket_error();
            if (errno == EMSGSIZE) continue;
        }
#else
        struct iovec iov;
        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));
        iov.iov_base = s->input; iov.iov_len = sizeof(s->input);
        msg.msg_iov = &iov; msg.msg_iovlen = 1;
        n = (int)recvmsg(s->fd, &msg, 0);
        if (n >= 0 && (msg.msg_flags & MSG_TRUNC)) continue;
#endif
        if (n < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            return -1;
        }
        len = (size_t)n;
#ifdef KCP_ENABLE_CRYPTO
        if (s->crypt) {
            int result = kcp_crypt_open(s->crypt, data, len, s->plain, sizeof(s->plain), &len);
            if (result < 0) return -1;
            if (result > 0) continue;
            data = s->plain;
        }
#endif
        /* KCP commands do not collide with the two FEC type markers. */
        if (len >= FEC_HEADER_SIZE && data[5] == 0 &&
            (data[4] == FEC_TYPE_DATA || data[4] == FEC_TYPE_PARITY)) {
            int result;
            if (!s->rx_fec) {
                s->rx_fec = fec_create(1, 1);
                if (!s->rx_fec) return -1;
            }
            result = fec_input(s->rx_fec, data, len, now, input_kcp, s);
            if (result < 0) return -1;
        } else input_kcp(s, data, len);
    }
    s->kcp->current = now;
    ikcp_flush(s->kcp);
    if (s->output_error) { errno = s->output_error; return -1; }
    return 0;
}

/* Copy ready KCP bytes into a caller buffer without blocking. */
ssize_t kcp_session_read(kcp_session *s, char *buf, size_t size) {
    int n, peek;
    if (!s || (!buf && size)) { errno = EINVAL; return -1; }
    if (!size) return 0;
    if (s->stream_size) {
        size_t take = s->stream_size < size ? s->stream_size : size;
        memcpy(buf, s->stream, take);
        s->stream_size -= take;
        memmove(s->stream, s->stream + take, s->stream_size);
        return (ssize_t)take;
    }
    /* Retain the remainder when the caller's buffer is smaller than a message. */
    peek = ikcp_peeksize(s->kcp);
    if (peek <= 0) return 0;
    if ((size_t)peek <= size) return ikcp_recv(s->kcp, buf, peek);
    {
        char *p = (char *)realloc(s->stream, (size_t)peek);
        if (!p) return -1;
        s->stream = p;
    }
    n = ikcp_recv(s->kcp, s->stream, peek);
    if (n != peek) { errno = EIO; return -1; }
    memcpy(buf, s->stream, size);
    s->stream_size = (size_t)peek - size;
    memmove(s->stream, s->stream + size, s->stream_size);
    return (ssize_t)size;
}

/* Queue bytes for transmission during a subsequent update. */
ssize_t kcp_session_write(kcp_session *s, const char *buf, size_t size) {
    int result;
    if (!s || (!buf && size) || size > INT_MAX) { errno = EINVAL; return -1; }
    result = ikcp_send(s->kcp, buf, (int)size);
    return result == 0 ? (ssize_t)size : result;
}

/* Set the IP differentiated services code point for the connected socket. */
int kcp_session_set_dscp(kcp_session *s, int dscp) {
    int family, value, level, option;
    struct sockaddr_storage addr;
    kcp_socklen len = sizeof(addr);
    if (!s || dscp < 0 || dscp > 63) { errno = EINVAL; return -1; }
    if (getsockname(s->fd, (struct sockaddr *)&addr, &len)) { errno = socket_error(); return -1; }
    family = ((struct sockaddr *)&addr)->sa_family;
    value = dscp << 2;
    level = family == AF_INET6 ? IPPROTO_IPV6 : IPPROTO_IP;
    option = family == AF_INET6 ? IPV6_TCLASS : IP_TOS;
    if (setsockopt(s->fd, level, option, (const char *)&value, sizeof(value))) {
        errno = socket_error(); return -1;
    }
    return 0;
}
/* Select KCP stream or message mode. */
void kcp_session_set_stream(kcp_session *s, int enabled) { if (s) s->kcp->stream = !!enabled; }
/* Configure KCP's low-latency and congestion-control options. */
int kcp_session_nodelay(kcp_session *s, int n, int i, int r, int nc) {
    if (!s) { errno = EINVAL; return -1; }
    return ikcp_nodelay(s->kcp, n, i, r, nc);
}
/* Configure the local KCP send and receive windows. */
int kcp_session_wndsize(kcp_session *s, int send_window, int receive_window) {
    if (!s) { errno = EINVAL; return -1; }
    return ikcp_wndsize(s->kcp, send_window, receive_window);
}
/* Apply a KCP MTU that leaves room for FEC and encryption headers. */
int kcp_session_set_mtu(kcp_session *s, int mtu) {
    size_t limit = FEC_MAX_PACKET, overhead = s && s->fec ? FEC_DATA_HEADER_SIZE : 0;
    int result;
    if (!s || mtu < 0) { errno = EINVAL; return -1; }
#ifdef KCP_ENABLE_CRYPTO
    if (s->crypt) { limit = 1472; overhead += kcp_crypt_overhead(s->crypt); }
#endif
    if (overhead > limit || (size_t)mtu > limit - overhead) { errno = EMSGSIZE; return -1; }
    result = ikcp_setmtu(s->kcp, mtu);
    if (result) { errno = result == -2 ? ENOMEM : EINVAL; return -1; }
    return 0;
}
/* Return a wrapping millisecond clock for frame-driven updates. */
uint32_t kcp_current_ms(void) {
#ifdef _WIN32
    return (uint32_t)GetTickCount64();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t)((uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000);
#endif
}
