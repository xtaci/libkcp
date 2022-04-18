//
// Created by 理 傅 on 2016/12/30.
//

#ifndef KCP_GALOIS_NOASM_H
#define KCP_GALOIS_NOASM_H

#include "galois.h"

#ifdef __cplusplus
extern "C" {
#endif
void galMulSlice(byte c,  byte* in, byte* out, int size);
void galMulSliceXor(byte c, byte* in, byte* out, int size);

#ifdef __cplusplus
}
#endif

#endif //KCP_GALOIS_NOASM_H
