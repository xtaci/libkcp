#include "sess.h"
#include "encoding.h"
#include <iostream>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>

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

    UDPSession *sess = UDPSession::createSession(sockfd);
    if (sess == nullptr) {
        close(sockfd);
        return nullptr;
    }
    return sess;
}

UDPSession *
UDPSession::DialWithOptions(const char *ip, uint16_t port, int dataShards, int parityShards) {
    auto sess = UDPSession::Dial(ip, port);
    if (sess == nullptr) {
        return nullptr;
    }

    if (dataShards >0 && parityShards > 0) {
        sess->fec = FEC::New(3*(dataShards + parityShards), dataShards, parityShards);
        sess->shards.resize(dataShards + parityShards);
        sess->dataShards = dataShards;
        sess->parityShards = parityShards;
    }
    return sess;
};


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

    UDPSession *sess = UDPSession::createSession(sockfd);
    if (sess == nullptr) {
        close(sockfd);
        return nullptr;
    }
    return sess;
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
    sess->m_buf = (char *) malloc(UDPSession::mtuLimit);
    sess->m_streambuf = (char *) malloc(UDPSession::streamBufferLimit);
    sess->m_kcp->output = sess->out_wrapper;

    if (sess->m_kcp == nullptr || sess->m_buf == nullptr || sess->m_streambuf == nullptr) {
        if (nullptr != sess->m_kcp) { ikcp_release(sess->m_kcp); }
        if (nullptr != sess->m_buf) { free(sess->m_buf); }
        if (nullptr != sess->m_streambuf) { free(sess->m_streambuf); }
        return nullptr;
    }
    return sess;
}


void
UDPSession::Update(uint32_t current) noexcept {
    for (;;) {
        ssize_t n = recv(m_sockfd, m_buf, UDPSession::mtuLimit, 0);
        if (n > 0) {
            if (fec.isEnabled()) {
                auto pkt = fec.Decode(m_buf, n);
                if (pkt.flag == typeData) {
                    auto ptr = pkt.data->data();
                    ikcp_input(m_kcp, (char *)(ptr+ 2), pkt.data->size() - 2);
                }

                auto recovered = fec.Input(pkt);

                if (recovered.size() > 0) {
                    for (int i =0;i<recovered.size();i++) {
                        auto ptr = recovered[i]->data();
                        uint16_t sz;
                        decode16u((char*)ptr, &sz);
                        ikcp_input(m_kcp, (char *)(ptr+ 2), sz);
                    }
                }
            } else {
                ikcp_input(m_kcp, (char *)(m_buf), n);
            }
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
    if (nullptr != sess->m_kcp) { ikcp_release(sess->m_kcp); }
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
    if (psz <= 0) {
        return 0;
    }

    if (psz <= sz) {
        return (ssize_t) ikcp_recv(m_kcp, buf, int(sz));
    } else {
        ikcp_recv(m_kcp, m_streambuf, UDPSession::streamBufferLimit);
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
    return setsockopt(this->m_sockfd, IPPROTO_IP, IP_TOS,  &iptos, sizeof(iptos));
}

int
UDPSession::out_wrapper(const char *buf, int len, struct IKCPCB *, void *user) {
    assert(user != nullptr);
    UDPSession *sess = static_cast<UDPSession *>(user);

    if (sess->fec.isEnabled()) {
        // extend to len + fecHeaderSizePlus2
        memcpy(sess->m_buf+fecHeaderSizePlus2, buf, len);
        sess->fec.MarkData(sess->m_buf, len);
        sess->output(sess->m_buf, len + fecHeaderSizePlus2);
    } else {
        sess->output(buf, static_cast<size_t>(len));
    }

    if (sess->fec.isEnabled()) {
        sess->shards[sess->pkt_idx] =
                std::make_shared<std::vector<byte>>(sess->m_buf + fecHeaderSize, sess->m_buf + len + fecHeaderSizePlus2);

        sess->pkt_idx++;
        if (sess->pkt_idx == sess->dataShards) {
            for (size_t i = sess->dataShards;i<sess->dataShards+sess->parityShards;i++) {
                sess->shards[i] = nullptr;
            }

            auto ret = sess->fec.Encode(sess->shards);
            if (ret == 0 ) {
                for (size_t i = sess->dataShards;i<sess->dataShards+sess->parityShards;i++) {
                    memcpy(sess->m_buf+fecHeaderSize, sess->shards[i]->data(), sess->shards[i]->size());
                    sess->fec.MarkFEC(sess->m_buf);
                    sess->output(sess->m_buf, sess->shards[i]->size() + fecHeaderSize);
                }
            }
            sess->pkt_idx = 0;
        }
    }

    return 0;
}

ssize_t
UDPSession::output(const void *buffer, size_t length) {
    ssize_t n = send(m_sockfd, buffer, length, 0);
    return n;
}
