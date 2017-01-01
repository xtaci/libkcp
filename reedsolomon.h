//
// Created by 理 傅 on 2017/1/1.
//

#ifndef KCP_REEDSOLOMON_H
#define KCP_REEDSOLOMON_H

#include "matrix.h"
#include "inversion_tree.h"

class ReedSolomon {
public:
    ReedSolomon(const ReedSolomon &) = delete;
    ReedSolomon &operator=(const ReedSolomon &) = delete;

    static ReedSolomon * New(int dataShards, int parityShards);
private:
    int DataShards;  // Number of data shards, should not be modified.
    int ParityShards; // Number of parity shards, should not be modified.
    int Shards; // Total number of shards. Calculated, and should not be modified.

    matrix *m;
    inversionTree *tree;
    uint8_t **parity;

    ReedSolomon() = default;
    ~ReedSolomon() = default;
};



#endif //KCP_REEDSOLOMON_H
