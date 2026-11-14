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
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

/* Verify reconstruction for multiple erasure patterns and the shard limit. */
int main(void) {
    uint8_t bytes[8][193], original[8][193], present[8];
    uint8_t *shards[8];
    reedsolomon *rs;
    int i, j, a, b, c;
    assert(!rs_create(0, 1));
    assert(!rs_create(255, 2));
    rs = rs_create(5, 3);
    assert(rs);
    for (i = 0; i < 8; ++i) {
        shards[i] = bytes[i];
        for (j = 0; j < 193; ++j) bytes[i][j] = (uint8_t)(i * 41 + j * 13);
    }
    assert(rs_encode(rs, shards, 193) == 0);
    memcpy(original, bytes, sizeof(bytes));
    for (a = 0; a < 8; ++a) for (b = a + 1; b < 8; ++b) for (c = b + 1; c < 8; ++c) {
        memset(present, 1, sizeof(present));
        present[a] = present[b] = present[c] = 0;
        memset(bytes[a], 0, 193); memset(bytes[b], 0, 193); memset(bytes[c], 0, 193);
        assert(rs_reconstruct(rs, shards, present, 193) == 0);
        assert(memcmp(bytes, original, sizeof(bytes)) == 0);
    }
    memset(present, 0, sizeof(present));
    assert(rs_reconstruct(rs, shards, present, 193) == -1);
    assert(rs_encode(rs, shards, 0) == 0);
    rs_destroy(rs);
    {
        uint8_t values[256], expected[255], flags[256], *large[256];
        rs = rs_create(255, 1);
        assert(rs);
        for (i = 0; i < 256; ++i) {
            values[i] = (uint8_t)(i * 13);
            large[i] = &values[i];
            flags[i] = 1;
        }
        memcpy(expected, values, sizeof(expected));
        assert(!rs_encode(rs, large, 1));
        flags[254] = 0;
        values[254] = 0;
        assert(!rs_reconstruct(rs, large, flags, 1));
        assert(!memcmp(values, expected, sizeof(expected)));
        rs_destroy(rs);
    }
    return 0;
}
