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

#ifndef KCP_AUTOTUNE_H
#define KCP_AUTOTUNE_H
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AUTOTUNE_SAMPLES 258

/* One observed FEC type and its sequence number. */
typedef struct {
    uint32_t seq; /* Wrapping wire sequence number. */
    int data;     /* Nonzero for a data shard, zero for parity. */
} autotune_sample;

/* Recent observations in arrival order. */
typedef struct {
    autotune_sample samples[AUTOTUNE_SAMPLES]; /* Fixed-capacity ring. */
    size_t head;  /* Index of the oldest sample. */
    size_t count; /* Number of valid samples. */
} autotune;

/* Append a sample, evicting the oldest when the ring is full. */
void autotune_add(autotune *tune, int data, uint32_t seq);
/* Return the complete pulse width, or -1 if boundaries are ambiguous. */
int autotune_period(const autotune *tune, int data);
#ifdef __cplusplus
}
#endif
#endif
