//
// Created by 理 傅 on 2017/1/2.
//

#ifndef KCP_FEC_H
#define KCP_FEC_H


#include <cstdint>
#include <vector>
#include "reedsolomon.h"

class fecPacket {
public:
    uint32_t seqid;
    uint16_t flag;
    char *data;
    uint32_t ts;
    ~fecPacket();
};

class FEC {
public:
    static FEC *newFEC(int rxlimit, int dataShards, int parityShards);
    FEC(const FEC &) = delete;
    FEC &operator=(const FEC &) = delete;

    fecPacket decode(char* data, size_t sz);
    void markData(char * data);
    void markFEC(char * data);

    int input(fecPacket pkt, uint8_t ** shards, size_t *numShards, int *shardSize);
private:
    FEC() = default;
    ~FEC() = default;

    std::vector<fecPacket> rx; // ordered receive queue
    int rxlimit;  // queue size limit
    int dataShards;
    int parityShards;
    int shardSize;
    uint32_t next{0}; // next seqid
    ReedSolomon * enc;
    char **shards;
    std::vector<bool> shardsflag;
    uint32_t paws;  // Protect Against Wrapped Sequence numbers
    uint32_t lastCheck{0};

    static const size_t fecHeaderSize{6};
    static const size_t fecHeaderSizePlus2{fecHeaderSize+2};
    static const char typeData = 0xf1;
    static const char typeFEC = 0xf2;
    static const int fecExpire{30000};
};


#endif //KCP_FEC_H
