//
// Created by 理 傅 on 16/7/9.
//

#ifndef KCP_SESS_H
#define KCP_SESS_H

#include "ikcp.h"


class Session {
private:
    int m_sockfd;
    ikcpcb *kcp;
public:
    static Session *DialKCP(char *network, char *address);
    // Update pools udp socket and input into kcp;
    int Update(char * buf, size_t sz);
    // PeekSize() returns the buffer size need to allocate for Read()
    size_t PeekSize();
    // Read reads from kcp and writes to buf and returns num of bytes written.
    size_t Read(char * buf, size_t sz);
};

#endif //KCP_SESS_H
