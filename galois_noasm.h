//
// Created by 理 傅 on 2016/12/30.
//

#ifndef KCP_GALOIS_NOASM_H
#define KCP_GALOIS_NOASM_H
#include <stddef.h>
#include <stdint.h>
#include "galois.h"

#ifdef __cplusplus
extern "C" {
#endif
void galMulSlice(byte c, byte in[], byte out[], size_t sz);
void galMulSliceXor(byte c, byte in[], byte out[],size_t sz);

#ifdef __cplusplus
}
#endif

#endif //KCP_GALOIS_NOASM_H
