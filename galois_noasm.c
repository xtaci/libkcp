//
// Created by 理 傅 on 2016/12/30.
//

#include <stddef.h>
#include "galois_noasm.h"
#include "galois.h"

extern const byte mulTable[256][256];

void galMulSlice(byte c, byte in[], byte out[], size_t sz) {
    for (int n=0;n<sz;n++) {
        out[n] = mulTable[c][in[n]];
    }
}

void galMulSliceXor(byte c, byte in[], byte out[], size_t sz) {
    for (int n=0;n<sz;n++) {
        out[n] ^= mulTable[c][in[n]];
    }
}
