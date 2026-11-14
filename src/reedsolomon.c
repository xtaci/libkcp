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

#include "reedsolomon.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

extern unsigned char mulTable[256][256];
extern unsigned char logTable[];
extern unsigned char expTable[];

/* Systematic generator matrix for one shard layout. */
struct reedsolomon {
    int data, parity, total; /* Counts of data, parity and all shards. */
    uint8_t *matrix;         /* Row-major GF(256) generator matrix. */
};

/* Exponentiate a field element using precomputed logarithms. */
static uint8_t power(uint8_t n, int p) {
    if (!p) return 1;
    if (!n) return 0;
    return expTable[(logTable[n] * p) % 255];
}

/* Invert a square GF(256) matrix by Gauss-Jordan elimination. */
static int invert(uint8_t *a, uint8_t *inverse, int n) {
    int r, c, k, row;
    memset(inverse, 0, (size_t)n * n);
    for (r = 0; r < n; ++r) inverse[r * n + r] = 1;
    for (r = 0; r < n; ++r) {
        if (!a[r * n + r]) {
            for (row = r + 1; row < n && !a[row * n + r]; ++row) {}
            if (row == n) { errno = EINVAL; return -1; }
            for (c = 0; c < n; ++c) {
                uint8_t t = a[r * n + c];
                a[r * n + c] = a[row * n + c]; a[row * n + c] = t;
                t = inverse[r * n + c];
                inverse[r * n + c] = inverse[row * n + c]; inverse[row * n + c] = t;
            }
        }
        {
            uint8_t scale = expTable[(255 - logTable[a[r * n + r]]) % 255];
            for (c = 0; c < n; ++c) {
                a[r * n + c] = mulTable[scale][a[r * n + c]];
                inverse[r * n + c] = mulTable[scale][inverse[r * n + c]];
            }
        }
        for (row = 0; row < n; ++row) {
            uint8_t scale;
            if (row == r || !(scale = a[row * n + r])) continue;
            for (k = 0; k < n; ++k) {
                a[row * n + k] ^= mulTable[scale][a[r * n + k]];
                inverse[row * n + k] ^= mulTable[scale][inverse[r * n + k]];
            }
        }
    }
    return 0;
}

/* Build a systematic codec for the requested data and parity counts. */
reedsolomon *rs_create(int data, int parity) {
    reedsolomon *rs;
    uint8_t *top, *inv;
    int r, c, k;
    if (data <= 0 || parity <= 0 || data > 255 || parity > 256 - data) {
        errno = EINVAL; return NULL;
    }
    rs = (reedsolomon *)calloc(1, sizeof(*rs));
    if (!rs) return NULL;
    rs->data = data; rs->parity = parity; rs->total = data + parity;
    rs->matrix = (uint8_t *)malloc((size_t)rs->total * data);
    top = (uint8_t *)malloc((size_t)data * data);
    inv = (uint8_t *)malloc((size_t)data * data);
    if (!rs->matrix || !top || !inv) {
        free(top); free(inv); rs_destroy(rs); errno = ENOMEM; return NULL;
    }
    /* Normalize a Vandermonde matrix so its data rows form the identity. */
    for (r = 0; r < data; ++r)
        for (c = 0; c < data; ++c) top[r * data + c] = power((uint8_t)r, c);
    if (invert(top, inv, data)) { free(top); free(inv); rs_destroy(rs); return NULL; }
    for (r = 0; r < rs->total; ++r)
        for (c = 0; c < data; ++c) {
            uint8_t value = 0;
            for (k = 0; k < data; ++k)
                value ^= mulTable[power((uint8_t)r, k)][inv[k * data + c]];
            rs->matrix[r * data + c] = value;
        }
    free(top); free(inv);
    return rs;
}

/* Free the generator matrix and its codec. */
void rs_destroy(reedsolomon *rs) {
    if (rs) { free(rs->matrix); free(rs); }
}

/* Apply one generator or inverse row to equally sized shards. */
static void code(const uint8_t *row, uint8_t **input, int n, uint8_t *out, size_t len) {
    int i;
    size_t j;
    if (len) memset(out, 0, len);
    for (i = 0; i < n; ++i)
        for (j = 0; j < len; ++j) out[j] ^= mulTable[row[i]][input[i][j]];
}

/* Generate parity from a complete set of data shards. */
int rs_encode(reedsolomon *rs, uint8_t **shards, size_t len) {
    int i;
    if (!rs || !shards) { errno = EINVAL; return -1; }
    for (i = 0; i < rs->total; ++i)
        if (!shards[i]) { errno = EINVAL; return -1; }
    for (i = rs->data; i < rs->total; ++i)
        code(rs->matrix + i * rs->data, shards, rs->data, shards[i], len);
    return 0;
}

/* Reconstruct absent shards from any sufficient available subset. */
int rs_reconstruct(reedsolomon *rs, uint8_t **shards, const uint8_t *present, size_t len) {
    uint8_t *work, *inv, **inputs;
    int i, count = 0, missing = 0, d;
    if (!rs || !shards || !present) { errno = EINVAL; return -1; }
    d = rs->data;
    for (i = 0; i < rs->total; ++i) {
        if (!shards[i]) { errno = EINVAL; return -1; }
        count += !!present[i]; missing += !present[i];
    }
    if (count < d) { errno = EINVAL; return -1; }
    if (!missing) return 0;
    work = (uint8_t *)malloc((size_t)d * d);
    inv = (uint8_t *)malloc((size_t)d * d);
    inputs = (uint8_t **)malloc((size_t)d * sizeof(*inputs));
    if (!work || !inv || !inputs) {
        free(work); free(inv); free(inputs); errno = ENOMEM; return -1;
    }
    /* Invert the rows for the first available data-count shards. */
    count = 0;
    for (i = 0; i < rs->total && count < d; ++i)
        if (present[i]) {
            memcpy(work + count * d, rs->matrix + i * d, d);
            inputs[count++] = shards[i];
        }
    if (invert(work, inv, d)) { free(work); free(inv); free(inputs); return -1; }
    /* Restore data before computing any missing parity from that data. */
    for (i = 0; i < d; ++i)
        if (!present[i]) code(inv + i * d, inputs, d, shards[i], len);
    for (i = d; i < rs->total; ++i)
        if (!present[i]) code(rs->matrix + i * d, shards, d, shards[i], len);
    free(work); free(inv); free(inputs);
    return 0;
}
