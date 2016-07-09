//
// Created by 理 傅 on 16/7/9.
//

#include "sess.h"
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

UDPSession *
UDPSession::Dial(const char *ip, uint16_t port) {
    int sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        return 0;
    }
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = inet_addr(ip);
    memset(&saddr.sin_zero, 0, 8);

    if (connect(sockfd, (struct sockaddr *) &saddr, sizeof(struct sockaddr)) < 0) {
        return 0;
    }

    void *buf = malloc(UDPSession::mtuLimit);
    if (buf == 0) {
        return 0;
    }

    UDPSession *sess = new(UDPSession);
    sess->m_sockfd = sockfd;
    sess->m_kcp = ikcp_create(IUINT32(rand()), sess);
    sess->m_kcp->output = sess->out_wrapper;
    sess->m_buf = buf;
    return sess;
}

void
UDPSession::Update() {
    while (1) {
        ssize_t ret = recv(m_sockfd, m_buf, m_bufsiz, 0);
        if (ret > 0) {
            ikcp_input(m_kcp, static_cast<const char *>(m_buf), ret);
        } else {
            break;
        }
    }
    ikcp_update(m_kcp, iclock());
}

void
UDPSession::Close() {
    if (m_sockfd != 0) { close(m_sockfd); }
    if (m_buf != 0) { free(m_buf); }
    if (m_kcp != 0) { ikcp_release(m_kcp); }
    delete this;
}

int
UDPSession::out_wrapper(const char *buf, int len, struct IKCPCB *kcp, void *user) {
    UDPSession *sess = static_cast<UDPSession *>(user);
    return sess->output(buf, len);
}

ssize_t
UDPSession::output(const void *buffer, size_t length) {
    return ::send(m_sockfd, buffer, length, 0);
}

void
UDPSession::itimeofday(long *sec, long *usec) {
#if defined(__unix)
    struct timeval time;
    gettimeofday(&time, NULL);
    if (sec) *sec = time.tv_sec;
    if (usec) *usec = time.tv_usec;
#else
    static long mode = 0, addsec = 0;
    BOOL retval;
    static IINT64 freq = 1;
    IINT64 qpc;
    if (mode == 0) {
        retval = QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
        freq = (freq == 0)? 1 : freq;
        retval = QueryPerformanceCounter((LARGE_INTEGER*)&qpc);
        addsec = (long)time(NULL);
        addsec = addsec - (long)((qpc / freq) & 0x7fffffff);
        mode = 1;
    }
    retval = QueryPerformanceCounter((LARGE_INTEGER*)&qpc);
    retval = retval * 2;
    if (sec) *sec = (long)(qpc / freq) + addsec;
    if (usec) *usec = (long)((qpc % freq) * 1000000 / freq);
#endif
}

IUINT64
UDPSession::iclock64(void) {
    long s, u;
    IUINT64 value;
    itimeofday(&s, &u);
    value = ((IUINT64) s) * 1000 + (u / 1000);
    return value;
}

IUINT32
UDPSession::iclock() {
    return (IUINT32) (iclock64() & 0xfffffffful);
}


