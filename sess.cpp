//
// Created by 理 傅 on 16/7/9.
//

#include "sess.h"
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>
#include <string.h>


UDPSession * UDPSession::Dial(const char * ip, uint16_t port){
    int sockfd = socket(PF_INET, SOCK_DGRAM ,0);
    if(sockfd == -1){
        return 0;
    }
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in saddr;
    saddr.sin_family    = AF_INET;
    saddr.sin_port      = htons(port);
    saddr.sin_addr.s_addr = inet_addr(ip);
    memset(&saddr.sin_zero, 0, 8);

    UDPSession *sess = new(UDPSession);
    sess->m_sockfd = sockfd;
    sess->m_kcp = ikcp_create(IUINT32(rand()), sess);

    return sess;
}

size_t UDPSession::PeekSize() {
    return (size_t)ikcp_peeksize(this->m_kcp);
}

int UDPSession::Read(char * buf, size_t sz) {
    return ikcp_recv(this->m_kcp, buf, int(sz));
}

int UDPSession::Write(const char * buf, size_t sz) {
    return ikcp_send(this->m_kcp, const_cast<char*>(buf), int(sz));
}

