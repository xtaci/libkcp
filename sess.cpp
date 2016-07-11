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

    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
        close(sockfd);
        return nullptr;
    }

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(sockfd);
        return nullptr;
    }

    void *buf = malloc(UDPSession::mtuLimit);
    if (buf == nullptr) {
        close(sockfd);
        return nullptr;
    }

    UDPSession *sess = new UDPSession;
    auto kcp = ikcp_create(IUINT32(rand()), sess);
    sess->m_sockfd = sockfd;
    sess->m_kcp = kcp;
    sess->m_kcp->output = sess->out_wrapper;
    sess->m_buf = buf;
    sess->m_bufsiz = UDPSession::mtuLimit;
    if (kcp == nullptr) {
        UDPSession::Destroy(sess);
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

    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
        close(sockfd);
        return nullptr;
    }

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(sockfd);
        return nullptr;
    }

    void *buf = malloc(UDPSession::mtuLimit);
    if (buf == nullptr) {
        close(sockfd);
        return nullptr;
    }

    UDPSession *sess = new UDPSession;
    auto kcp = ikcp_create(IUINT32(rand()), sess);
    sess->m_sockfd = sockfd;
    sess->m_kcp = kcp;
    sess->m_kcp->output = sess->out_wrapper;
    sess->m_buf = buf;
    sess->m_bufsiz = UDPSession::mtuLimit;
    if (kcp == nullptr) {
        UDPSession::Destroy(sess);
        return nullptr;
    }
    return sess;
}

void
UDPSession::Update(uint32_t current) {
    for(;;) {
        ssize_t n = recv(m_sockfd, m_buf, m_bufsiz, 0);
        if (n > 0) {
            ikcp_input(m_kcp, static_cast<const char *>(m_buf), n);
        } else {
            break;
        }
    }
    ikcp_update(m_kcp, current);
}

void
UDPSession::Destroy(UDPSession * sess){
    if (sess->m_sockfd != 0) { close(sess->m_sockfd); }
    if (sess->m_buf != 0) { free(sess->m_buf); }
    if (sess->m_kcp != 0) { ikcp_release(sess->m_kcp); }
    delete sess;
}

int
UDPSession::out_wrapper(const char *buf, int len, struct IKCPCB *, void *user) {
    assert(user != nullptr);
    UDPSession *sess = static_cast<UDPSession *>(user);
    return (int)sess->output(buf, static_cast<size_t>(len));
}

ssize_t
UDPSession::output(const void *buffer, size_t length) {
    ssize_t n = send(m_sockfd, buffer, length, 0);
    return n;
}