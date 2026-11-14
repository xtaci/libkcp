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

#ifndef KCP_CRYPT_BACKEND_H
#define KCP_CRYPT_BACKEND_H
/* Internal backend interface; applications should use crypt.h instead. */
#include "crypt.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Create the backend and expand its key; NULL indicates an error. */
void *kcp_crypto_create(kcp_crypt_algorithm algorithm, const uint8_t *key, size_t keylen);
/* Release backend state and erase stored keys. */
void kcp_crypto_destroy(void *ctx);
/* Fill out with cryptographically secure system randomness. */
int kcp_crypto_random(void *ctx, uint8_t *out, size_t len);
/* Apply the legacy packet transform in place. */
int kcp_crypto_transform(void *ctx, uint8_t *data, size_t len, int encrypt);
/* Encrypt or authenticate and decrypt an AES-GCM payload in place. */
int kcp_crypto_gcm(void *ctx, uint8_t *data, size_t len, const uint8_t *nonce,
                   uint8_t *tag, int encrypt);
#ifdef __cplusplus
}
#endif
#endif
