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

#ifndef KCP_CRYPT_H
#define KCP_CRYPT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Packet formats supported by kcp-go; SM4 is reserved but unavailable. */
typedef enum {
    KCP_CRYPT_AES_GCM, KCP_CRYPT_AES, KCP_CRYPT_SM4, KCP_CRYPT_TWOFISH,
    KCP_CRYPT_3DES, KCP_CRYPT_CAST5, KCP_CRYPT_BLOWFISH, KCP_CRYPT_TEA,
    KCP_CRYPT_XTEA, KCP_CRYPT_SALSA20, KCP_CRYPT_XOR, KCP_CRYPT_NONE
} kcp_crypt_algorithm;

typedef struct kcp_crypt kcp_crypt;

/* A context belongs to one session and must outlive it. NULL means failure;
 * requesting SM4 returns NULL with errno set to ENOTSUP. */
kcp_crypt *kcp_crypt_create(kcp_crypt_algorithm algorithm, const uint8_t *key, size_t keylen);
/* Custom kcp-go AEAD: callbacks operate in place. seal returns 0/-1; open
 * returns 0 for success, 1 for authentication failure, -1 for other errors.
 * Callbacks must set errno on operational failure.
 * The callback context remains owned by the caller and must outlive crypt. */
typedef int (*kcp_aead_seal_fn)(void *user, const uint8_t *nonce, size_t nonce_len,
                                uint8_t *data, size_t len, uint8_t *tag, size_t tag_len);
typedef int (*kcp_aead_open_fn)(void *user, const uint8_t *nonce, size_t nonce_len,
                                uint8_t *data, size_t len, const uint8_t *tag, size_t tag_len);
kcp_crypt *kcp_crypt_create_aead(size_t nonce_len, size_t tag_len,
                                 kcp_aead_seal_fn seal, kcp_aead_open_fn open, void *user);
/* Release a context created by either constructor. */
void kcp_crypt_destroy(kcp_crypt *crypt);
/* Return the per-packet nonce, tag or legacy framing overhead in bytes. */
size_t kcp_crypt_overhead(const kcp_crypt *crypt);

/* Return 0 on success, 1 for an invalid packet, -1 for an operational error. */
int kcp_crypt_seal(kcp_crypt *crypt, const uint8_t *data, size_t len,
                   uint8_t *out, size_t capacity, size_t *outlen);
/* Authenticate and decrypt a complete packet before exposing its plaintext. */
int kcp_crypt_open(kcp_crypt *crypt, const uint8_t *packet, size_t len,
                   uint8_t *out, size_t capacity, size_t *outlen);

#ifdef __cplusplus
}
#endif
#endif
