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

#ifndef KCP_FEC_H
#define KCP_FEC_H
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
#define FEC_HEADER_SIZE 6
#define FEC_DATA_HEADER_SIZE 8
#define FEC_TYPE_DATA 0xf1
#define FEC_TYPE_PARITY 0xf2
#define FEC_MAX_PACKET 2048

/* FEC encoder and independently auto-tuned decoder. */
typedef struct fec fec;
/* Consume a packet synchronously; nonzero reports an output error. */
typedef int (*fec_output_fn)(void *user, const uint8_t *data, size_t len);

/* Sending uses these shard counts; receiving automatically detects peer counts.
   Both counts must be positive and their sum must not exceed 256. */
fec *fec_create(int data_shards, int parity_shards);
/* Release a FEC context and any pending shard buffers. */
void fec_destroy(fec *f);
/* Callbacks receive complete FEC packets on output and KCP packets on input. */
int fec_encode(fec *f, const uint8_t *data, size_t len, fec_output_fn emit, void *user);
/* 0: processed, 1: malformed packet, -1: operational error. */
int fec_input(fec *f, const uint8_t *data, size_t len, uint32_t now,
              fec_output_fn emit, void *user);
#ifdef __cplusplus
}
#endif
#endif
