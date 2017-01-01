//
// Created by 理 傅 on 2017/1/1.
//

#include <cstdlib>
#include <stddef.h>
#include "reedsolomon.h"
#include "matrix.h"

ReedSolomon *
ReedSolomon::New(int dataShards, int parityShards) {
    if (dataShards<=0 || parityShards <=0) {
        return nullptr;
    }

    if (dataShards + parityShards > 255) {
        return nullptr;
    }

    ReedSolomon * r = new ReedSolomon;
    r->DataShards = dataShards;
    r->ParityShards = parityShards;
    r->Shards = dataShards + parityShards;

    // Start with a Vandermonde matrix.  This matrix would work,
    // in theory, but doesn't have the property that the data
    // shards are unchanged after encoding.
    matrix * vm  = matrix::vandermonde(r->Shards, r->DataShards);
    if (vm == nullptr){
        return nullptr;
    }

    // Multiply by the inverse of the top square of the matrix.
    // This will make the top square be the identity matrix, but
    // preserve the property that any square subset of rows  is
    // invertible.
    auto top = vm->SubMatrix(0,0, dataShards,dataShards);
    top = top->Invert();
    r->m = vm->Multiply(top);

    // Inverted matrices are cached in a tree keyed by the indices
    // of the invalid rows of the data to reconstruct.
    // The inversion root node will have the identity matrix as
    // its inversion matrix because it implies there are no errors
    // with the original data.
    r->tree = inversionTree::newInversionTree(dataShards, parityShards);

    r->parity = (uint8_t**)malloc(sizeof(uint8_t*) * parityShards);
    for (int i=0;i<parityShards;i++) {
        r->parity[i] = r->m->m[dataShards+i];
    }
    return r;
}

