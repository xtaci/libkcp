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

#include "autotune.h"

/* Record an observed shard type in the fixed-size ring. */
void autotune_add(autotune *tune, int data, uint32_t seq) {
    size_t tail = (tune->head + tune->count) % AUTOTUNE_SAMPLES;
    tune->samples[tail].data = !!data;
    tune->samples[tail].seq = seq;
    if (tune->count < AUTOTUNE_SAMPLES) ++tune->count;
    else tune->head = (tune->head + 1) % AUTOTUNE_SAMPLES;
}

/* Compare sequence numbers around a wrapping 32-bit anchor. */
static int64_t relative_seq(uint32_t seq, uint32_t anchor) {
    uint32_t delta = seq - anchor;
    return delta < 0x80000000u ? (int64_t)delta : (int64_t)delta - 0x100000000LL;
}

/* Find a complete, contiguous pulse of data or parity observations. */
int autotune_period(const autotune *tune, int data) {
    autotune_sample sorted[AUTOTUNE_SAMPLES];
    size_t i, j, unique = 0;
    uint32_t anchor;
    if (tune->count < 3) return -1;
    /* Sort a copy so out-of-order arrival does not reorder the sample ring. */
    anchor = tune->samples[tune->head].seq;
    for (i = 0; i < tune->count; ++i) {
        autotune_sample current = tune->samples[(tune->head + i) % AUTOTUNE_SAMPLES];
        j = i;
        while (j > 0 && relative_seq(sorted[j - 1].seq, anchor) > relative_seq(current.seq, anchor)) {
            sorted[j] = sorted[j - 1];
            --j;
        }
        sorted[j] = current;
    }
    /* Collapse identical duplicates and discard conflicting observations. */
    for (i = 0; i < tune->count;) {
        size_t end = i + 1;
        int conflict = 0;
        while (end < tune->count && sorted[end].seq == sorted[i].seq) {
            if (sorted[end].data != sorted[i].data) conflict = 1;
            ++end;
        }
        if (!conflict) sorted[unique++] = sorted[i];
        i = end;
    }
    /* A pulse is valid only when both edges and every intervening sample exist. */
    data = !!data;
    for (i = 1; i < unique; ++i) {
        if (sorted[i - 1].data == data || sorted[i].data != data ||
            sorted[i - 1].seq + 1 != sorted[i].seq) continue;
        for (j = i + 1; j < unique; ++j) {
            if (sorted[j - 1].seq + 1 != sorted[j].seq) break;
            if (sorted[j].data != data) return (int)(sorted[j].seq - sorted[i].seq);
        }
    }
    return -1;
}
