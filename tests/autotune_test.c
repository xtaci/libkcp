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
#include <assert.h>
#include <stdint.h>

/* Check gaps, duplicates, reordering, wraparound and the sample limit. */
int main(void) {
    autotune tune = {0};
    int i;
    autotune_add(&tune, 0, 10);
    autotune_add(&tune, 1, 12);
    autotune_add(&tune, 0, 13);
    assert(autotune_period(&tune, 1) == -1);
    autotune_add(&tune, 1, 11);
    autotune_add(&tune, 1, 11);
    assert(autotune_period(&tune, 1) == 2);
    autotune_add(&tune, 0, 11);
    assert(autotune_period(&tune, 1) == -1);
    autotune_add(&tune, 1, 14);
    autotune_add(&tune, 0, 15);
    assert(autotune_period(&tune, 1) == 1);
    assert(autotune_period(&tune, 0) == 1);
    tune = (autotune){0};
    autotune_add(&tune, 0, UINT32_MAX - 1);
    autotune_add(&tune, 1, 0);
    autotune_add(&tune, 0, 1);
    autotune_add(&tune, 1, UINT32_MAX);
    assert(autotune_period(&tune, 1) == 2);
    tune = (autotune){0};
    autotune_add(&tune, 0, 0);
    for (i = 1; i <= 255; ++i) autotune_add(&tune, 1, (uint32_t)i);
    autotune_add(&tune, 0, 256);
    autotune_add(&tune, 1, 257);
    assert(autotune_period(&tune, 1) == 255);
    assert(autotune_period(&tune, 0) == 1);
    autotune_add(&tune, 1, 258);
    assert(autotune_period(&tune, 1) == -1);
    return 0;
}
