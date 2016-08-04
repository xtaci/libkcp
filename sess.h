//
// Created by 理 傅 on 16/7/9.
//

#ifndef KCP_SESS_H
#define KCP_SESS_H

#include "ikcp.h"
#include "io.h"
#include <sys/types.h>

class UDPSession : public ReadWriter {
private:
    int m_sockfd{0};
    ikcpcb *m_kcp{nullptr};
    char *m_buf{nullptr};
    char *m_streambuf{nullptr};
    size_t m_streambufsiz{0};
public:
    UDPSession(const UDPSession &) = delete;

    UDPSession &operator=(const UDPSession &) = delete;

    // Dial connects to the remote server and returns UDPSession.
    static UDPSession *Dial(const char *ip, uint16_t port);

    // Update will try reading/writing udp packet, pass current unix millisecond
    void Update(uint32_t current) noexcept;

    // Destroy release all resource related.
    static void Destroy(UDPSession *sess);

    // Read reads from kcp with buffer size sz.
    ssize_t Read(char *buf, size_t sz) noexcept;

    // Write writes into kcp with buffer size sz.
    ssize_t Write(const char *buf, size_t sz) noexcept;

    // Wrappers for kcp control
    inline int NoDelay(int nodelay, int interval, int resend, int nc) {
        return ikcp_nodelay(m_kcp, nodelay, interval, resend, nc);
    }

    inline int WndSize(int sndwnd, int rcvwnd) { return ikcp_wndsize(m_kcp, sndwnd, rcvwnd); }

    inline int SetMtu(int mtu) { return ikcp_setmtu(m_kcp, mtu); }

private:
    UDPSession() = default;

    ~UDPSession() = default;

    // DialIPv6 is the ipv6 version of Dial.
    static UDPSession *dialIPv6(const char *ip, uint16_t port);

    // out_wrapper
    static int out_wrapper(const char *buf, int len, struct IKCPCB *kcp, void *user);

    // output udp packet
    ssize_t output(const void *buffer, size_t length);

    static UDPSession *createSession(int sockfd);

private:
    static const size_t mtuLimit{2048};
    static const size_t streamBufferLimit{65535};
};

#endif //KCP_SESS_H
