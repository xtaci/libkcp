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

#include "galois.h"
#include <errno.h>
extern unsigned char mulTable[256][256];
extern unsigned char logTable[];
extern unsigned char expTable[];

/* Addition and subtraction in characteristic two are XOR. */
byte galAdd(byte a, byte b) { return a ^ b; }
/* Subtraction is identical to addition in this field. */
byte galSub(byte a, byte b) { return a ^ b; }
/* Use the precomputed multiplication table for the two field elements. */
byte galMultiply(byte a, byte b) { return mulTable[a][b]; }
/* Divide through logarithms; zero divisors report EDOM. */
byte galDivide(byte a, byte b) {
    if (!b) { errno = EDOM; return 0; }
    if (!a) return 0;
    return expTable[(logTable[a] + 255 - logTable[b]) % 255];
}
/* Raise a field element to an unsigned integer power. */
byte galExp(byte a, byte n) {
    if (!n) return 1;
    if (!a) return 0;
    return expTable[(logTable[a] * n) % 255];
}
