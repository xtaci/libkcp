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

#ifndef KCP_GALOIS_H
#define KCP_GALOIS_H

typedef unsigned char byte;

/* Add two GF(256) elements. */
byte galAdd(byte a, byte b);

/* Subtract two GF(256) elements. */
byte galSub(byte a, byte b);

// galMultiply multiplies to elements of the field.
// Uses lookup table ~40% faster
byte galMultiply(byte a, byte b);

// galDivide is inverse of galMultiply.
byte galDivide(byte a, byte b);

// Computes a**n.
//
// The result will be the same as multiplying a times itself n times.
byte galExp(byte a, byte b);

#endif //KCP_GALOIS_H
