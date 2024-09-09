//
// Created by 理 傅 on 2016/12/30.
//

#include "galois_noasm.h"

extern const byte mulTable[256][256];

void galMulSlice(byte c, byte* in, byte* out, int size) {
  for (int n = 0; n < size; n++) {
    out[n] = mulTable[c][in[n]];
  }
}

void galMulSliceXor(byte c, byte* in, byte* out, int size) {
  for (int n = 0; n < size; n++) {
    out[n] ^= mulTable[c][in[n]];
  }
}