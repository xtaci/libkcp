//
// Created by 理 傅 on 16/7/9.
//

#ifndef KCP_SESS_H
#define KCP_SESS_H

#include "ikcp.h"

#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
#include <windows.h>
#elif !defined(__unix)
#define __unix
#endif


class UDPSession {
private:
    int m_sockfd;
    ikcpcb *m_kcp;
public:
    static UDPSession *Dial(const char *ip, uint16_t port);

    // Call Update() periodically for sending/receiving.
    inline void Update() { ikcp_update(m_kcp, iclock()); }

    // PeekSize() returns the buffer size need to allocate for Read()
    inline size_t PeekSize() { return (size_t) ikcp_peeksize(this->m_kcp); }

    // Read reads from kcp with buffer size sz
    inline int Read(char *buf, size_t sz) { return ikcp_recv(this->m_kcp, buf, int(sz)); }

    // Write writes into kcp with buffer size sz
    inline int Write(const char *buf, size_t sz) { return ikcp_send(this->m_kcp, const_cast<char *>(buf), int(sz)); }

private:
    void itimeofday(long *sec, long *usec);

    /* get clock in millisecond 64 */
    int64_t iclock64();

    /* get clock in millisecond 32 */
    int32_t iclock();
};

#endif //KCP_SESS_H
