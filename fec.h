//
// Created by 理 傅 on 2017/1/2.
//

#ifndef KCP_FEC_H
#define KCP_FEC_H


#include <vector>
#include <stdint.h>
#include <memory>
#include "reedsolomon.h"

const size_t fecHeaderSize = 6;
const size_t fecHeaderSizePlus2{fecHeaderSize + 2};
const uint16_t typeData = 0xf1;
const uint16_t typeFEC = 0xf2;
const int fecExpire = 30000;

class fecPacket {
public:
    uint32_t seqid{0};
    uint16_t flag{0};
    row_type data;
    uint32_t ts{0};
};

class FEC {
public:
    FEC() = default;
    FEC(ReedSolomon enc);

    static FEC New(int rxlimit, int dataShards, int parityShards);

    inline bool isEnabled() const { return dataShards > 0 && parityShards > 0; }

    // Add a received shard and return any data shards recovered from its group.
    std::vector<row_type> Input(fecPacket &pkt);

    // Generate parity shards from the data shards in the provided shard set.
    void Encode(std::vector<row_type> &shards);

    // Decode a complete FEC packet; throws std::invalid_argument if its header is truncated.
    static fecPacket Decode(byte *data, size_t sz);

    // Write a data-shard header and its encoded payload size.
    void MarkData(byte *data, uint16_t sz);

    // Write a parity-shard header and advance the shard sequence number.
    void MarkFEC(byte *data);
private:
    std::vector<fecPacket> rx; // Received shards, ordered by sequence number.
    int rxlimit{0};  // Maximum number of received shards retained.
    int dataShards{0};
    int parityShards{0};
    int totalShards{0};
    uint32_t next{0}; // next seqid
    ReedSolomon enc;
    uint32_t paws{0};  // Sequence-number limit before wrapping.
    uint32_t lastCheck{0};
};


#endif //KCP_FEC_H
