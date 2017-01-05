//
// Created by 理 傅 on 2017/1/2.
//

#ifndef KCP_FEC_H
#define KCP_FEC_H


#include <vector>
#include <memory>
#include "reedsolomon.h"

const size_t fecHeaderSize = 6;
const size_t fecHeaderSizePlus2{fecHeaderSize + 2};
const uint16_t typeData = 0xf1;
const uint16_t typeFEC = 0xf2;
const int fecExpire = 30000;
const int mtuLimit = 2048;

class fecPacket {
public:
    uint32_t seqid;
    uint16_t flag;
    row data;
    uint32_t ts;
};

class FEC {
public:
    FEC(ReedSolomon enc);

    static FEC New(int rxlimit, int dataShards, int parityShards);

    // Input a FEC packet, and return recovered data if possible.
    std::vector<row> Input(fecPacket &pkt);

    // Calc Parity Shards
    inline int Encode(std::vector<row> &shards) { return enc.Encode(shards); }

    // Decode a raw array into fecPacket
    static fecPacket Decode(char *data, size_t sz);

    // Mark raw array as typeData
    void MarkData(char *data);

    // Mark raw array as typeFEC
    void MarkFEC(char *data);
private:
    std::vector<fecPacket> rx; // ordered receive queue
    int rxlimit;  // queue empty limit
    int dataShards;
    int parityShards;
    int totalShards;
    uint32_t next{0}; // next seqid
    ReedSolomon enc;
    uint32_t paws;  // Protect Against Wrapped Sequence numbers
    uint32_t lastCheck{0};
};


#endif //KCP_FEC_H
