#ifndef KCP_SESS_H
#define KCP_SESS_H

#include "ikcp.h"
#include "fec.h"
#include <sys/types.h>
#include <sys/time.h>
#include <Network/Network.h>
#include <err.h>
#import "BlockCrypt.h"
typedef void (^recvBlock)(char* buffer,size_t len);
void
dump(char *tag, char *text, int len);
class UDPSession  {
private:
    int m_sockfd{0};
    //size_t buffer_used;
    nw_connection_t outbound_connection = NULL;
    ikcpcb *m_kcp{nullptr};
    byte m_buf[2048];
    byte m_streambuf[65535];
    size_t m_streambufsiz{0};

    FEC fec;
    uint32_t pkt_idx{0};
    std::vector<row_type> shards;
    size_t dataShards{0};
    size_t parityShards{0};
    BlockCrypt *block;
 
public:
    UDPSession(const UDPSession &) = delete;

    UDPSession &operator=(const UDPSession &) = delete;

    // Dial connects to the remote server and returns UDPSession.
    static UDPSession *Dial(const char *ip, uint16_t port);

    // DialWithOptions connects to the remote address "raddr" on the network "udp" with packet encryption
    static UDPSession *DialWithOptions(const char *ip, const char *port, size_t dataShards, size_t parityShards);

    // DialWithOptions connects to the remote address "raddr" on the network "udp" with packet encryption with block
    static UDPSession *DialWithOptions(const char *ip, const char *port, size_t dataShards, size_t parityShards,BlockCrypt *block);
    // Update will try reading/writing udp packet, pass current unix millisecond
    void Update(uint32_t current) noexcept;
    void NWUpdate(uint32_t current) noexcept;
    //kcpInput go UDPpsession
    void KcpInPut(size_t len) noexcept;
    // Destroy release all resource related.
    static void Destroy(UDPSession *sess);

    // Read reads from kcp with buffer empty sz.
    ssize_t Read(char *buf, size_t sz) noexcept;

    // Write writes into kcp with buffer empty sz.
    ssize_t Write(const char *buf, size_t sz) noexcept;

    // Set DSCP value
    int SetDSCP(int dscp) noexcept;

    char *getLocalIPAddr() noexcept;
    recvBlock didRecv;
    int getLocalPort() noexcept;
    // SetStreamMode toggles the stream mode on/off
    void SetStreamMode(bool enable) noexcept;

    // Wrappers for kcp control
    inline int NoDelay(int nodelay, int interval, int resend, int nc) {
        return ikcp_nodelay(m_kcp, nodelay, interval, resend, nc);
    }

    inline int WndSize(int sndwnd, int rcvwnd) { return ikcp_wndsize(m_kcp, sndwnd, rcvwnd); }

    inline int SetMtu(int mtu) { return ikcp_setmtu(m_kcp, mtu); }
    void receive_loop();
    void start_send_receive_loop(recvBlock didRecv);
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
    static UDPSession *createSession(nw_connection_t sockfd);
    //new
    static nw_connection_t create_outbound_connection(const char *, const char *);
    void start_connection(nw_connection_t connection);
    
    void send_loop(nw_connection_t connection,dispatch_data_t _Nonnull read_data);
   
};

inline uint32_t currentMs() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return uint32_t((time.tv_sec * 1000) + (time.tv_usec / 1000));
}


#endif //KCP_SESS_H
