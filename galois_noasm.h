//
// Created by 理 傅 on 2016/12/30.
//

#ifndef KCP_GALOIS_NOASM_H
#define KCP_GALOIS_NOASM_H

#include "galois.h"
#include "matrix.h"

#ifdef __cplusplus
extern "C" {
#endif
void galMulSlice(byte c,  row in, row out);
void galMulSliceXor(byte c, row in, row out);

#ifdef __cplusplus
}
#endif

#endif //KCP_GALOIS_NOASM_H
