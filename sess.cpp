#include "sess.h"
#include "encoding.h"
#include <iostream>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include "CRC32.h"
#include <Network/Network.h>

// Global Options nw
char *g_psk = NULL;            // TLS PSK
char *g_local_port = NULL;    // Local port flag
char *g_local_addr = NULL;    // Source Address
bool g_use_bonjour = false;    // Use Bonjour rather than hostnames
bool g_detached = true;    // Ignore stdin
bool g_listener = false;    // Create a listener
bool g_use_tls = false;        // Use TLS or DTLS
bool g_use_udp = true;        // Use UDP instead of TCP
bool g_verbose = false;        // Verbose
int g_family = AF_UNSPEC;     // Required address family
dispatch_queue_t q = dispatch_queue_create("nw.socket.queue",NULL);
#define NWCAT_BONJOUR_SERVICE_TCP_TYPE "_nwcat._tcp"
#define NWCAT_BONJOUR_SERVICE_UDP_TYPE "_nwcat._udp"
#define NWCAT_BONJOUR_SERVICE_DOMAIN "local"

//end
void
_itimeofday(long *sec, long *usec) {
    struct timeval time;
    gettimeofday(&time, NULL);
    if (sec) *sec = time.tv_sec;
    if (usec) *usec = time.tv_usec;
}
// 16-bytes magic number for each packet
const size_t nonceSize = 16;

// 4-bytes packet checksum
const size_t crcSize = 4;

// overall crypto header size
const size_t cryptHeaderSize = nonceSize + crcSize;

// maximum packet size
//const size_t mtuLimit = 1500;

// FEC keeps rxFECMulti* (dataShard+parityShard) ordered packets in memory
const size_t rxFECMulti = 3;
#define KCP_DEBUG 0
void
dump(char *tag,  byte *text, size_t len)
{
#if KCP_DEBUG
    int i;
    printf("%s: \n", tag);
    for (i = 0; i < len; i++){
        if ((i % 16) == 0 && i != 0){
            printf("\n");
        }
        if ((i % 4) == 0 && ((i%16) != 0)){
            printf(" ");
            
        }
        printf("%02x", (uint8_t)text[i]);
        
        
    }
    printf("\n");
#endif
}
#define debug_print(args ...) if (KCP_DEBUG) fprintf(stderr, args)
UDPSession *
UDPSession::Dial(const char *ip, uint16_t port) {
    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    int ret = inet_pton(AF_INET, ip, &(saddr.sin_addr));

    if (ret == 1) { // do nothing
    } else if (ret == 0) { // try ipv6
        return UDPSession::dialIPv6(ip, port);
    } else if (ret == -1) {
        return nullptr;
    }

    int sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    
    
   
    
    if (sockfd == -1) {
        return nullptr;
    }
    if (connect(sockfd, (struct sockaddr *) &saddr, sizeof(struct sockaddr)) < 0) {
        close(sockfd);
        return nullptr;
    }

    struct sockaddr_in localAddress;
    //socklen_t addressLength;
    socklen_t addressLength = sizeof(localAddress);
    getsockname(sockfd, (struct sockaddr*)&localAddress,   \
                &addressLength);
    printf("local address: %s\n", inet_ntoa( localAddress.sin_addr));
    printf("local port: %d\n", (int) ntohs(localAddress.sin_port));
    
    return UDPSession::createSession(sockfd);
}

UDPSession *
UDPSession::DialWithOptions(const char *ip, const char *port, size_t dataShards, size_t parityShards) {
    UDPSession  *sess;
    if (__builtin_available(iOS 12,macOS 10.14, *)) {
        printf("nw event base udp socket");
        
        nw_connection_t connection  = create_outbound_connection(ip , port);
        if (connection == NULL) {
            err(1, NULL);
        }
        sess = UDPSession::createSession(connection);
        sess->start_connection(connection);
        //
        
    }else {
        uint16_t arrToInt = 0;
        for(int i=0;i<=1;i++)
            arrToInt =(arrToInt<<8) | port[i];
        sess = UDPSession::Dial(ip, arrToInt);
        if (sess == nullptr) {
            return nullptr;
        }
    }
    
    

    if (dataShards > 0 && parityShards > 0) {
        sess->fec = FEC::New(rxFECMulti * (dataShards + parityShards), dataShards, parityShards);
        sess->shards.resize(dataShards + parityShards, nullptr);
        sess->dataShards = dataShards;
        sess->parityShards = parityShards;
    }
    
    sess->block = NULL;
    printf("sess->block:%p",sess->block);
    return sess;
};
// DialWithOptions connects to the remote address "raddr" on the network "udp" with packet encryption with block
UDPSession*
UDPSession::DialWithOptions(const char *ip, const char *port, size_t dataShards, size_t parityShards,BlockCrypt *block){
    auto sess = UDPSession::DialWithOptions(ip, port, dataShards, parityShards);
    if (sess == nullptr) {
        return nullptr;
    }
    
    sess->block = block;
    return sess;
}

UDPSession *
UDPSession::dialIPv6(const char *ip, uint16_t port) {
    struct sockaddr_in6 saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin6_family = AF_INET6;
    saddr.sin6_port = htons(port);
    if (inet_pton(AF_INET6, ip, &(saddr.sin6_addr)) != 1) {
        return nullptr;
    }

    int sockfd = socket(PF_INET6, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        return nullptr;
    }
    if (connect(sockfd, (struct sockaddr *) &saddr, sizeof(struct sockaddr_in6)) < 0) {
        close(sockfd);
        return nullptr;
    }

    return UDPSession::createSession(sockfd);
}

UDPSession *
UDPSession::createSession(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
        return nullptr;
    }

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        return nullptr;
    }

    UDPSession *sess = new(UDPSession);
    sess->m_sockfd = sockfd;
    sess->m_kcp = ikcp_create(IUINT32(rand()), sess);
    sess->m_kcp->output = sess->out_wrapper;
    
    return sess;
}
UDPSession *
UDPSession::createSession(nw_connection_t sockfd) {
    UDPSession *sess = new(UDPSession);
    sess->outbound_connection = sockfd;
    sess->m_kcp = ikcp_create(IUINT32(rand()), sess);
    sess->m_kcp->output = sess->out_wrapper;
    
    return sess;
}
//recv data from udp socket
///
void
UDPSession::Update(uint32_t current) noexcept {
    for (;;) {
        ssize_t n = recv(m_sockfd, m_buf, sizeof(m_buf), 0);
        if (n < 0) {
            //perror("read fopen( \"nulltest.txt\", \"r\" )");
            
            debug_print("kcp udp socket read error");
            //break;
        }
        if (n > 0) {
            dump((char*)"UDP Update", m_buf, n);
            //change by abigt
            bool dataValid = false;
            size_t outlen = n;
            //size_t orgsize = n;
            char *out = (char *)m_buf;
            //nonceSize = 16
            //outlen -= nonceSize;
            out += nonceSize;
            uint32_t sum = 0;
            if (block != NULL) {
                
                block->decrypt(m_buf, n, &outlen);
                
                memcpy(&sum, (uint8_t *)out, sizeof(uint32_t));
                out += crcSize;
                int32_t checksum = crc32_kr(0,(uint8_t *)out, n-cryptHeaderSize);
                if (checksum == sum){
                    dataValid = true;
                    
                }
            }else {
                
                memcpy(&sum, (uint8_t *)out, sizeof(uint32_t));
                out += crcSize;
                int32_t checksum = crc32_kr(0,(uint8_t *)out, n - cryptHeaderSize);
                if (checksum == sum){
                    dataValid = true;
                }
                
            }
            if (outlen != n) {
                debug_print("decrypt error outlen= %lu n = %lu\n",outlen,n);
            }
            if (dataValid == true) {
                memmove(m_buf, m_buf + cryptHeaderSize, n-cryptHeaderSize);
                KcpInPut(n - cryptHeaderSize);
            }
            
        } else {
            if (n == 0){
                debug_print("UDPSession recv nil");
            }
            break;
        }
    }
    m_kcp->current = current;
    ikcp_flush(m_kcp);
}
void
UDPSession::NWUpdate(uint32_t current) noexcept {
    m_kcp->current = current;
    ikcp_flush(m_kcp);
}
void
UDPSession::KcpInPut(size_t len) noexcept {
    if (fec.isEnabled()) {
        // decode FEC packet
        auto pkt = fec.Decode(m_buf, static_cast<size_t>(len));
        if (pkt.flag == kcptypeData) {
            auto ptr = pkt.data->data();
            // we have 2B size, ignore for typeData
            dump((char*)"ikcp input data:", (byte *) (ptr + 2),  pkt.data->size() - 2);
            ikcp_input(m_kcp, (char *) (ptr + 2), pkt.data->size() - 2);
        }
        
        // allow FEC packet processing with correct flags.
        if (pkt.flag == kcptypeData || pkt.flag == typeFEC) {
            // input to FEC, and see if we can recover data.
            auto recovered = fec.Input(pkt);
            
            // we have some data recovered.
            for (auto &r : recovered) {
                // recovered data has at least 2B size.
                if (r->size() > 2) {
                    auto ptr = r->data();
                    // decode packet size, which is also recovered.
                    uint16_t sz;
                    decode16u(ptr, &sz);
                    
                    // the recovered packet size must be in the correct range.
                    if (sz >= 2 && sz <= r->size()) {
                        // input proper data to kcp
                        dump((char*)"ikcp input data2:", (byte *) (ptr + 2), sz - 2);
                        ikcp_input(m_kcp, (char *) (ptr + 2), sz - 2);
                        
                        // std::cout << "sz:" << sz << std::endl;
                    }
                }
            }
        }
    } else { // fec disabled
        ikcp_input(m_kcp, (char *) (m_buf), len);
    }
}
void
UDPSession::Destroy(UDPSession *sess) {
    if (nullptr == sess) return;
    if (0 != sess->m_sockfd) { close(sess->m_sockfd); }
    if (nullptr != sess->m_kcp) { ikcp_release(sess->m_kcp); }
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
    if (psz <= 0) {
        return 0;
    }

    if (psz <= sz) {
        return (ssize_t) ikcp_recv(m_kcp, buf, int(sz));
    } else {
        ikcp_recv(m_kcp, (char *) m_streambuf, sizeof(m_streambuf));
        memcpy(buf, m_streambuf, sz);
        m_streambufsiz = psz - sz;
        memmove(m_streambuf, m_streambuf + sz, psz - sz);
        return sz;
    }
}

ssize_t
UDPSession::Write(const char *buf, size_t sz) noexcept {
    int n = ikcp_send(m_kcp, const_cast<char *>(buf), int(sz));
    if (n == 0) {
        return sz;
    } else return n;
}

int
UDPSession::SetDSCP(int iptos) noexcept {
    iptos = (iptos << 2) & 0xFF;
    return setsockopt(this->m_sockfd, IPPROTO_IP, IP_TOS, &iptos, sizeof(iptos));
}
char * UDPSession::getLocalIPAddr() noexcept{
    struct sockaddr_in localAddress;
    //socklen_t addressLength;
    socklen_t addressLength = sizeof(localAddress);
    getsockname(this->m_sockfd, (struct sockaddr*)&localAddress,   \
                &addressLength);
    printf("local address: %s\n", inet_ntoa( localAddress.sin_addr));
    printf("local port: %d\n", (int) ntohs(localAddress.sin_port));
    return inet_ntoa( localAddress.sin_addr);
}
int UDPSession::getLocalPort() noexcept{
    struct sockaddr_in localAddress;
    //socklen_t addressLength;
    socklen_t addressLength = sizeof(localAddress);
    getsockname(this->m_sockfd, (struct sockaddr*)&localAddress,   \
                &addressLength);
    printf("local address: %s\n", inet_ntoa( localAddress.sin_addr));
    printf("local port: %d\n", (int) ntohs(localAddress.sin_port));
    return (int) ntohs(localAddress.sin_port);
}
void
UDPSession::SetStreamMode(bool enable) noexcept {
    if (enable) {
        this->m_kcp->stream = 1;
    } else {
        this->m_kcp->stream = 0;
    }
}
// output pipeline entry
// steps for output data processing:
// 0. Header extends
// 1. FEC
// 2. CRC32
// 3. Encryption
// 4. WriteTo kernel
int
UDPSession::out_wrapper(const char *buf, int len, struct IKCPCB *, void *user) {
    assert(user != nullptr);
    UDPSession *sess = static_cast<UDPSession *>(user);
    //test no cover
    dump((char*)"UDPSession kcp packet", (byte*)buf, (size_t)len);
    if (sess->fec.isEnabled()) {    // append FEC header
        BlockCrypt *block = sess->block;
        
        
        // extend to len + fecHeaderSizePlus2
        // i.e. 4B seqid + 2B flag + 2B size
        //crypto none also add nonce and crc
        
        uint8_t *nonce = BlockCrypt::ramdonBytes(nonceSize);
        
        memcpy(sess->m_buf, nonce, nonceSize);
        free(nonce);
        byte *ptr = sess->m_buf + cryptHeaderSize;
        
        memcpy(ptr + fecHeaderSizePlus2, buf, static_cast<size_t>(len));
        sess->fec.MarkData(ptr, static_cast<uint16_t>(len));
        
        int32_t sum =  crc32_kr(0,ptr  ,len +  fecHeaderSizePlus2);
        memcpy(sess->m_buf + nonceSize, &sum, 4);
        //dump("UDPSession header", (char *)header, nonceSize + crcSize);
        //sess->output(header, nonceSize + crcSize );
        
        
        
        // FEC calculation
        // copy "2B size + data" to shards
        auto slen = len + 2;
        sess->shards[sess->pkt_idx] =
        std::make_shared<std::vector<byte>>(&sess->m_buf[fecHeaderSize + cryptHeaderSize], &sess->m_buf[fecHeaderSize + cryptHeaderSize + slen]);
        size_t outlen = 0;
        if (block != NULL ) {
            block->encrypt(sess->m_buf, len + fecHeaderSizePlus2 + cryptHeaderSize, &outlen);
            sess->output(sess->m_buf, outlen);
        }else {
            sess->output(sess->m_buf, len + fecHeaderSizePlus2 + cryptHeaderSize);
        }
        
        
        // count number of data shards
        sess->pkt_idx++;
        if (sess->pkt_idx == sess->dataShards) { // we've collected enough data shards
            sess->fec.Encode(sess->shards);
            // send parity shards
            //should add nonce and crc?
            
            for (size_t i = sess->dataShards; i < sess->dataShards + sess->parityShards; i++) {
                // append header to parity shards
                // i.e. fecHeaderSize + data(2B size included)
                // add nonce and crc
                uint8_t *nonce = BlockCrypt::ramdonBytes(nonceSize);
                
                memcpy(sess->m_buf, nonce, nonceSize);
                
                free(nonce);
                memcpy(ptr + fecHeaderSize, sess->shards[i]->data(), sess->shards[i]->size());
                sess->fec.MarkFEC(ptr);
                
                int32_t sum =  crc32_kr(0,ptr  ,sess->shards[i]->size()  +  fecHeaderSize);
                memcpy(sess->m_buf + nonceSize, &sum, 4);
                
                //go version write ecc to remote?
                if (block != NULL) {
                    block->encrypt(sess->m_buf, sess->shards[i]->size() + fecHeaderSize + cryptHeaderSize, &outlen);
                    sess->output(sess->m_buf,outlen);
                }else {
                    sess->output(sess->m_buf, sess->shards[i]->size() + fecHeaderSize + cryptHeaderSize);
                }
                
            }
            
            // reset indexing
            sess->pkt_idx = 0;
        }
        
        
        
        
    } else { // No FEC, just send raw bytes,
        //kcp-tun no use this
        sess->output(buf, static_cast<size_t>(len));
    }
    return 0;
}

ssize_t
UDPSession::output(const void *buffer, size_t length) {
    dump((char*)"UDPSession write socket", (byte *)buffer, length);
    if (__builtin_available(iOS 12,macOS 10.14, *)) {
        //send error check
        this->send_loop(this->outbound_connection, dispatch_data_create(buffer,length,nil,DISPATCH_DATA_DESTRUCTOR_DEFAULT));
        return length;
    }else {
        
        ssize_t n = send(m_sockfd, buffer, length, 0);
        if (n != length) {
            debug_print("not full send\n");
        }
        if (n==-1) {
            perror("send error fopen( \"nulltest.txt\", \"r\" )");
        }
        return n;
    }
    
    
}
/*
  * create_outbound_connection()
  * Returns a retained connection to a remote hostname and port.
  * Sets up TLS and local address/port as necessary.
  */
nw_connection_t
UDPSession::create_outbound_connection(const char *name, const char *port)
{
    // If we are using bonjour to connect, treat the name as a bonjour name
    // Otherwise, treat the name as a hostname
    if (__builtin_available(iOS 12,macOS 10.14, *)) {
        nw_endpoint_t endpoint = (g_use_bonjour ?
                                  nw_endpoint_create_bonjour_service(name,
                                                                     (g_use_udp ? NWCAT_BONJOUR_SERVICE_UDP_TYPE : NWCAT_BONJOUR_SERVICE_TCP_TYPE),
                                                                     NWCAT_BONJOUR_SERVICE_DOMAIN) :
                                  nw_endpoint_create_host(name, port));
        
        nw_parameters_t parameters = NULL;
        nw_parameters_configure_protocol_block_t configure_tls = NW_PARAMETERS_DISABLE_PROTOCOL;
        if (g_use_tls) {
            if (g_psk) {
                configure_tls = ^(nw_protocol_options_t tls_options) {
                    sec_protocol_options_t sec_options = nw_tls_copy_sec_protocol_options(tls_options);
                    dispatch_data_t psk = dispatch_data_create(g_psk, strlen(g_psk), nil, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
                    sec_protocol_options_add_pre_shared_key(sec_options, psk, psk);
                    dispatch_release(psk);
                    sec_protocol_options_add_tls_ciphersuite(sec_options, (SSLCipherSuite)TLS_PSK_WITH_AES_128_GCM_SHA256);
                    nw_release(sec_options);
                };
            } else {
                configure_tls = NW_PARAMETERS_DEFAULT_CONFIGURATION;
            }
        }
        
        if (g_use_udp) {
            // Create a UDP connection
            parameters = nw_parameters_create_secure_udp(configure_tls,
                                                         NW_PARAMETERS_DEFAULT_CONFIGURATION);
        } else {
            // Create a TCP connection
            parameters = nw_parameters_create_secure_tcp(configure_tls,
                                                         NW_PARAMETERS_DEFAULT_CONFIGURATION);
        }
        
        nw_protocol_stack_t protocol_stack = nw_parameters_copy_default_protocol_stack(parameters);
        if (g_family == AF_INET || g_family == AF_INET6) {
            nw_protocol_options_t ip_options = nw_protocol_stack_copy_internet_protocol(protocol_stack);
            if (g_family == AF_INET) {
                // Force IPv4
                nw_ip_options_set_version(ip_options, nw_ip_version_4);
            } else if (g_family == AF_INET6) {
                // Force IPv6
                nw_ip_options_set_version(ip_options, nw_ip_version_6);
            }
            nw_release(ip_options);
        }
        nw_release(protocol_stack);
        
        // Bind to local address and port
        if (g_local_addr || g_local_port) {
            nw_endpoint_t local_endpoint = nw_endpoint_create_host(g_local_addr ? g_local_addr : "::", g_local_port ? g_local_port : "0");
            nw_parameters_set_local_endpoint(parameters, local_endpoint);
            nw_release(local_endpoint);
        }
        
        nw_connection_t connection = nw_connection_create(endpoint, parameters);
        nw_release(endpoint);
        nw_release(parameters);
        
        return connection;
    } else {
        return  nil;
    }
    
    
}
void UDPSession::start_connection(nw_connection_t connection)
{
     if (__builtin_available(iOS 12, macOS 10.14,*)) {
         nw_connection_set_queue(connection,q);
         
         nw_retain(connection); // Hold a reference until cancelled
         nw_connection_set_state_changed_handler(connection, ^(nw_connection_state_t state, nw_error_t error) {
             nw_endpoint_t remote = nw_connection_copy_endpoint(connection);
             errno = error ? nw_error_get_error_code(error) : 0;
             if (state == nw_connection_state_waiting) {
                 warn("connect to %s port %u (%s) failed, is waiting",
                      nw_endpoint_get_hostname(remote),
                      nw_endpoint_get_port(remote),
                      g_use_udp ? "udp" : "tcp");
             } else if (state == nw_connection_state_failed) {
                 warn("connect to %s port %u (%s) failed",
                      nw_endpoint_get_hostname(remote),
                      nw_endpoint_get_port(remote),
                      g_use_udp ? "udp" : "tcp");
             } else if (state == nw_connection_state_ready) {
                 if (g_verbose) {
                     fprintf(stderr, "Connection to %s port %u (%s) succeeded!\n",
                             nw_endpoint_get_hostname(remote),
                             nw_endpoint_get_port(remote),
                             g_use_udp ? "udp" : "tcp");
                 }
             } else if (state == nw_connection_state_cancelled) {
                 // Release the primary reference on the connection
                 // that was taken at creation time
                 nw_release(connection);
             }
             nw_release(remote);
         });
         
         nw_connection_start(connection);
     }
    
}
/*
 * start_send_receive_loop()
 * Start reading from stdin (when not detached) and from the given connection.
 * Every read on stdin becomes a send on the connection, and every receive on the
 * connection becomes a write on stdout.
 */
void UDPSession::start_send_receive_loop()
{
    // Start reading from stdin
    //this->send_loop(connection);
    
    // Start reading from connection
    this->receive_loop();
}
/*
 * receive_loop()
 * Perform a single read on the supplied connection, and write data to
 * stdout as it is received.
 * If no error is encountered, schedule another read on the same connection.
 */
void
UDPSession::receive_loop()
{
    nw_connection_t connection = this->outbound_connection;
    printf("nw start recvloop");
    if (__builtin_available(iOS 12, macOS 10.14,*)) {
        nw_connection_receive(connection, 1, UINT32_MAX, ^(dispatch_data_t content, nw_content_context_t context, bool is_complete, nw_error_t receive_error) {
            
            CFRetain(context);
            dispatch_block_t schedule_next_receive = ^{
                // If the context is marked as complete, and is the final context,
                // we're read-closed.
                if (is_complete &&
                    
                    context != NULL && nw_content_context_get_is_final(context)) {
                    CFRelease(context);
                    exit(0);
                }
                
                // If there was no error in receiving, request more data
                if (receive_error == NULL) {
                    receive_loop();
                }
                CFRelease(context);
            };
            
            if (content != NULL) {
                // If there is content, write it to stdout asynchronously
                schedule_next_receive = Block_copy(schedule_next_receive);
                
                //dump((char*)"UDP Update", m_buf, n);
                //change by abigt
                bool dataValid = false;
                size_t outlen = 0;
                size_t n = dispatch_data_get_size(content);
                //size_t orgsize = n;
                memcpy(m_buf, content, n);
                char *out = (char *)m_buf;
                //nonceSize = 16
                //outlen -= nonceSize;
                out += nonceSize;
                uint32_t sum = 0;
                if (block != NULL) {
                    
                    block->decrypt(m_buf, n, &outlen);
                    
                    memcpy(&sum, (uint8_t *)out, sizeof(uint32_t));
                    out += crcSize;
                    int32_t checksum = crc32_kr(0,(uint8_t *)out, n-cryptHeaderSize);
                    if (checksum == sum){
                        dataValid = true;
                        
                    }
                }else {
                    
                    memcpy(&sum, (uint8_t *)out, sizeof(uint32_t));
                    out += crcSize;
                    int32_t checksum = crc32_kr(0,(uint8_t *)out, n - cryptHeaderSize);
                    if (checksum == sum){
                        dataValid = true;
                    }
                    
                }
                if (outlen != n) {
                    debug_print("decrypt error outlen= %lu n = %lu\n",outlen,n);
                }
                if (dataValid == true) {
                    memmove(m_buf, m_buf + cryptHeaderSize, n-cryptHeaderSize);
                    KcpInPut(n - cryptHeaderSize);
                }
                long s, u;
                IUINT64 value;
                
                struct timeval time;
                gettimeofday(&time, NULL);
                _itimeofday(&s, &u);
                value = ((IUINT64) s) * 1000 + (u / 1000);
                
                
                m_kcp->current = value & 0xfffffffful;
                ikcp_flush(m_kcp);
                
                dispatch_async( q, ^{
                   // Block_retain(schedule_next_receive);
                    schedule_next_receive();
                    Block_release(schedule_next_receive);
                });
            } else {
                // Content was NULL, so directly schedule the next receive
                schedule_next_receive();
            }
        });
    }
    
}
/*
 * send_loop()
 * Start reading from stdin on a dispatch source, and send any bytes on the given connection.
 */
void
UDPSession::send_loop(nw_connection_t connection, dispatch_data_t _Nonnull read_data){
    // Every send is marked as complete. This has no effect with the default message context for TCP,
    // but is required for UDP to indicate the end of a packet.
    printf("nw send data!\n");
    if (__builtin_available(iOS 12,macOS 10.14, *)) {
        nw_connection_send(connection, read_data, NW_CONNECTION_DEFAULT_MESSAGE_CONTEXT, true, ^(nw_error_t  _Nullable error) {
            if (error != NULL) {
                errno = nw_error_get_error_code(error);
                warn("send error");
            } else {
                // Continue reading from stdin
                //send_loop(connection);
            }
        });
    }
    
}
