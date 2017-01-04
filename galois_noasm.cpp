//
// Created by 理 傅 on 2016/12/30.
//

#include <stddef.h>
#include "galois_noasm.h"
#include "galois.h"
#include "matrix.h"

extern const byte mulTable[256][256];

void galMulSlice(byte c, row in, row out) {
    for (int n=0;n<in->size();n++) {
        (*out)[n] = mulTable[c][(*in)[n]];
    }
}

void galMulSliceXor(byte c, row in, row out) {
    for (int n=0;n<in->size();n++) {
        (*out)[n] ^= mulTable[c][(*in)[n]];
    }
}
