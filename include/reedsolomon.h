/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 xtaci
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef KCP_REEDSOLOMON_H
#define KCP_REEDSOLOMON_H
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
/* Systematic GF(256) Reed-Solomon codec. */
typedef struct reedsolomon reedsolomon;

/* Create a codec with positive counts totalling at most 256. */
reedsolomon *rs_create(int data_shards, int parity_shards);
/* Free the codec; NULL is accepted. */
void rs_destroy(reedsolomon *rs);
/* All shard buffers must have len bytes, including missing-shard output buffers. */
int rs_encode(reedsolomon *rs, uint8_t **shards, size_t len);
/* Recover missing data and parity shards; present contains one flag per shard. */
int rs_reconstruct(reedsolomon *rs, uint8_t **shards, const uint8_t *present, size_t len);
#ifdef __cplusplus
}
#endif
#endif
