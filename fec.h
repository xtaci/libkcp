//
// Created by 理 傅 on 2017/1/2.
//

#ifndef KCP_FEC_H
#define KCP_FEC_H


#include <cstdint>
#include <vector>
#include <memory>
#include "reedsolomon.h"

class fecPacket {
public:
    uint32_t seqid;
    uint16_t flag;
    char *data;
    size_t sz;
    uint32_t ts;
    ~fecPacket();
};

class FEC {
public:
    static FEC *newFEC(int rxlimit, int dataShards, int parityShards);
    FEC(const FEC &) = delete;
    FEC &operator=(const FEC &) = delete;

    fecPacket * decode(char* data, size_t sz);
    void markData(char * data);
    void markFEC(char * data);

    int input(fecPacket * pkt, std::vector<char *> *recovered);
private:
    FEC() = default;
    ~FEC() = default;

    std::vector<std::shared_ptr<fecPacket>> rx; // ordered receive queue
    int rxlimit;  // queue size limit
    int dataShards;
    int parityShards;
    int shardSize;
    uint32_t next{0}; // next seqid
    ReedSolomon * enc;
    uint32_t paws;  // Protect Against Wrapped Sequence numbers
    uint32_t lastCheck{0};

    static const size_t fecHeaderSize{6};
    static const size_t fecHeaderSizePlus2{fecHeaderSize+2};
    static const uint16_t typeData = 0xf1;
    static const uint16_t typeFEC = 0xf2;
    static const int fecExpire{30000};

    char **shardBuffer;
    static const int mtuLimit {2048};

    void zerobuffer();
};


#endif //KCP_FEC_H
