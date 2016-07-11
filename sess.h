//
// Created by 理 傅 on 16/7/9.
//

#ifndef KCP_SESS_H
#define KCP_SESS_H

#include "ikcp.h"
#include <sys/types.h>

class UDPSession {
private:
    int m_sockfd;
    ikcpcb *m_kcp;
    void *m_buf;
    size_t m_bufsiz;
public:
    UDPSession(const UDPSession &) = delete;

    UDPSession &operator=(const UDPSession &) = delete;

    // Dial connects to the remote server and returns UDPSession.
    static UDPSession *Dial(const char *ip, uint16_t port);

    // DialIPv6 is the ipv6 version of Dial.
    static UDPSession *DialIPv6(const char *ip, uint16_t port);

    // Update will try reading/writing udp packet, pass current unix millisecond
    void Update(uint32_t current);

    // Close release all resource related.
    static void Destroy(UDPSession *sess);

    // PeekSize returns the buffer size needed for reading.
    inline size_t PeekSize() noexcept { return (size_t) ikcp_peeksize(m_kcp); }

    // Read reads from kcp with buffer size sz.
    inline ssize_t Read(char *buf, size_t sz) { return (ssize_t) ikcp_recv(m_kcp, buf, int(sz)); }

    // Write writes into kcp with buffer size sz.
    inline ssize_t Write(const char *buf, size_t sz) {
        return (ssize_t) ikcp_send(m_kcp, const_cast<char *>(buf), int(sz));
    }

private:
    UDPSession() : m_sockfd(0), m_kcp(nullptr), m_buf(nullptr), m_bufsiz(0) { };
    ~UDPSession() = default;

    // out_wrapper
    static int out_wrapper(const char *buf, int len, struct IKCPCB *kcp, void *user);

    // output udp packet
    ssize_t output(const void *buffer, size_t length);
private:
    static const size_t mtuLimit = 4096;
};

#endif //KCP_SESS_H
