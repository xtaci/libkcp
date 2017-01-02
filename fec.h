//
// Created by 理 傅 on 2017/1/2.
//

#ifndef KCP_FEC_H
#define KCP_FEC_H


#include <cstdint>
#include <vector>
#include "reedsolomon.h"

struct fecPacket {
    uint32_t seqid;
    uint16_t flag;
    char *data;
    uint32_t ts;
};

class FEC {
public:
    static FEC *newFEC(int rxlimit, int dataShards, int parityShards);
    FEC(const FEC &) = delete;
    FEC &operator=(const FEC &) = delete;

    fecPacket decode(char* data, size_t sz);

private:
    FEC() = default;
    ~FEC() = default;

    std::vector<fecPacket> rx; // ordered receive queue
    int rxlimit;  // queue size limit
    int dataShards;
    int parityShards;
    int shardSize;
    uint32_t next; // next seqid
    ReedSolomon * enc;
    char **shards;
    std::vector<bool> shardsflag;
    uint32_t paws;  // Protect Against Wrapped Sequence numbers
    uint32_t lastCheck;
};


#endif //KCP_FEC_H
