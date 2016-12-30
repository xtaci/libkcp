//
// Created by 理 傅 on 2016/12/30.
//

#include <stddef.h>
#include "galois_noasm.h"

extern const uint8_t mulTable[256][256];

void galMulSlice(uint8_t c, uint8_t in[], uint8_t out[], size_t sz) {
    for (int i=0;i<sz;i++) {
        out[i] = mulTable[c][in[i]];
    }
}

void galMulSliceXor(uint8_t c, uint8_t in[], uint8_t out[], size_t sz) {
    for (int i=0;i<sz;i++) {
        out[i] ^= mulTable[c][in[i]];
    }
}
