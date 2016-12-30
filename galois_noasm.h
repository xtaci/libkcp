//
// Created by 理 傅 on 2016/12/30.
//

#ifndef KCP_GALOIS_NOASM_H
#define KCP_GALOIS_NOASM_H
#include <stddef.h>
#include <stdint.h>

void galMulSlice(uint8_t c, uint8_t in[], uint8_t out[], size_t sz);
void galMulSliceXor(uint8_t c, uint8_t in[], uint8_t out[],size_t sz);

#endif //KCP_GALOIS_NOASM_H
