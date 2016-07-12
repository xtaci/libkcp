//
// Created by 理 傅 on 16/7/9.
//

#include "sess.h"
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

UDPSession *
UDPSession::Dial(const char *ip, uint16_t port) {
    int sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        return nullptr;
    }

    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    inet_aton(ip, &(saddr.sin_addr));
    if (connect(sockfd, (struct sockaddr *) &saddr, sizeof(struct sockaddr)) < 0) {
        close(sockfd);
        return nullptr;
    }

    UDPSession *sess = new UDPSession;
    if (!UDPSession::init(sess,sockfd)) {
        close(sockfd);
        return nullptr;
    }
    return sess;
}

UDPSession *
UDPSession::DialIPv6(const char *ip, uint16_t port) {
    int sockfd = socket(PF_INET6, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        return nullptr;
    }
    struct sockaddr_in6 saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin6_family = AF_INET6;
    saddr.sin6_port = htons(port);
    inet_pton(AF_INET6, ip, &(saddr.sin6_addr));

    if (connect(sockfd, (struct sockaddr *) &saddr, sizeof(struct sockaddr_in6)) < 0) {
        close(sockfd);
        return nullptr;
    }

    UDPSession *sess = new UDPSession;
    if (!UDPSession::init(sess,sockfd)) {
        close(sockfd);
        return nullptr;
    }
    return sess;
}

bool
UDPSession::init(UDPSession *sess, int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        return false;
    }

    sess->m_sockfd = sockfd;
    sess->m_kcp = ikcp_create(IUINT32(rand()), sess);
    sess->m_buf = (char*)malloc(UDPSession::mtuLimit);
    sess->m_streambuf =  (char*)malloc(UDPSession::streamBufferLimit);
    sess->m_kcp->output = sess->out_wrapper;

    if (sess->m_kcp == nullptr || sess->m_buf == nullptr || sess->m_streambuf == nullptr) {
        UDPSession::Destroy(sess);
        return false;
    }
    return true;
}


void
UDPSession::Update(uint32_t current) noexcept {
    for (; ;) {
        ssize_t n = recv(m_sockfd, m_buf, UDPSession::mtuLimit, 0);
        if (n > 0) {
            ikcp_input(m_kcp, static_cast<const char *>(m_buf), n);
        } else {
            break;
        }
    }
    ikcp_update(m_kcp, current);
}

void
UDPSession::Destroy(UDPSession *sess) {
    if (nullptr == sess) return;
    if (0 != sess->m_sockfd) { close(sess->m_sockfd); }
    if (0 != sess->m_kcp) { ikcp_release(sess->m_kcp); }
    if (nullptr != sess->m_buf) { free(sess->m_buf); }
    if (nullptr != sess->m_streambuf) { free(sess->m_streambuf); }
    delete sess;
}

ssize_t
UDPSession::Read(char *buf, size_t sz) noexcept {
    if (m_streambufsiz > 0) {
        size_t n = m_streambufsiz;
        if (n > sz) {
            n = sz;
        }
        memcpy(buf, m_streambuf, n);


        m_streambufsiz -= n;
        if (m_streambufsiz != 0) {
            memmove(m_streambuf, m_streambuf + n, m_streambufsiz);
        }
        return n;
    }

    int psz = ikcp_peeksize(m_kcp);
    if (psz<=0) {
        return 0;
    }

    if (psz <= sz) {
        return (ssize_t) ikcp_recv(m_kcp, buf, int(sz));
    } else {
        ikcp_recv(m_kcp, m_streambuf, UDPSession::streamBufferLimit);
        memcpy(buf, m_streambuf, psz);
        m_streambufsiz = psz-sz;
        memmove(m_streambuf, m_streambuf + psz, psz-sz);
        return sz;
    }
}


int
UDPSession::out_wrapper(const char *buf, int len, struct IKCPCB *, void *user) {
    assert(user != nullptr);
    UDPSession *sess = static_cast<UDPSession *>(user);
    return (int) sess->output(buf, static_cast<size_t>(len));
}

ssize_t
UDPSession::output(const void *buffer, size_t length) {
    ssize_t n = send(m_sockfd, buffer, length, 0);
    return n;
}