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

#include "crypt.h"
#include "crypt_backend.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PACKET 1472
#define LEGACY_HEADER 20

/* Per-session packet framing and its cipher or custom AEAD callbacks. */
struct kcp_crypt {
    kcp_crypt_algorithm algorithm; /* Built-in packet encryption mode. */
    void *backend;                 /* Owned crypto and random-number state. */
    size_t nonce_len, tag_len;     /* Custom AEAD nonce and tag sizes. */
    kcp_aead_seal_fn seal;         /* Optional custom encryption callback. */
    kcp_aead_open_fn open;         /* Optional custom authentication callback. */
    void *user;                    /* Borrowed callback context. */
};

/* Compute the legacy kcp-go CRC, which detects errors but is not a MAC. */
static uint32_t crc32_ieee(const uint8_t *data, size_t len) {
    uint32_t crc = 0xffffffffu;
    size_t i;
    int bit;
    for (i = 0; i < len; ++i) {
        crc ^= data[i];
        for (bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

/* Create the requested built-in packet cipher. */
kcp_crypt *kcp_crypt_create(kcp_crypt_algorithm algorithm, const uint8_t *key, size_t keylen) {
    kcp_crypt *ctx;
    if ((!key && keylen) || algorithm < KCP_CRYPT_AES_GCM || algorithm > KCP_CRYPT_NONE) {
        errno = EINVAL;
        return NULL;
    }
    ctx = (kcp_crypt *)calloc(1, sizeof(*ctx));
    if (!ctx) return NULL;
    ctx->algorithm = algorithm;
    ctx->backend = kcp_crypto_create(algorithm, key, keylen);
    if (!ctx->backend) {
        free(ctx);
        return NULL;
    }
    return ctx;
}

/* Create a packet cipher backed by caller-supplied AEAD callbacks. */
kcp_crypt *kcp_crypt_create_aead(size_t nonce_len, size_t tag_len,
                                 kcp_aead_seal_fn seal, kcp_aead_open_fn open, void *user) {
    kcp_crypt *ctx;
    if (nonce_len > MAX_PACKET ||
        tag_len > MAX_PACKET - nonce_len || !seal || !open) {
        errno = EINVAL; return NULL;
    }
    ctx = kcp_crypt_create(KCP_CRYPT_NONE, NULL, 0);
    if (!ctx) return NULL;
    ctx->nonce_len = nonce_len; ctx->tag_len = tag_len;
    ctx->seal = seal; ctx->open = open; ctx->user = user;
    return ctx;
}

/* Release owned backend state while leaving callback user data untouched. */
void kcp_crypt_destroy(kcp_crypt *ctx) {
    if (ctx) {
        kcp_crypto_destroy(ctx->backend);
        free(ctx);
    }
}

/* Compute the encoded packet's framing overhead. */
size_t kcp_crypt_overhead(const kcp_crypt *ctx) {
    if (ctx->seal) return ctx->nonce_len + ctx->tag_len;
    return ctx->algorithm == KCP_CRYPT_AES_GCM ? 28 : LEGACY_HEADER;
}

/* Encode and encrypt one kcp-go-compatible packet. */
int kcp_crypt_seal(kcp_crypt *ctx, const uint8_t *data, size_t len,
                   uint8_t *out, size_t capacity, size_t *outlen) {
    size_t overhead;
    uint32_t crc;
    int i;
    if (!ctx || (!data && len) || !out || !outlen) { errno = EINVAL; return -1; }
    overhead = kcp_crypt_overhead(ctx);
    if (len > MAX_PACKET - overhead || capacity < len + overhead) { errno = EMSGSIZE; return -1; }
    /* kcp-go uses nonce || ciphertext || tag for AEAD and nonce || CRC
     * || payload for legacy encryption. */
    if (ctx->seal) {
        if (len) memmove(out + ctx->nonce_len, data, len);
        if (kcp_crypto_random(ctx->backend, out, ctx->nonce_len)) return -1;
        if (ctx->seal(ctx->user, out, ctx->nonce_len, out + ctx->nonce_len, len,
                      out + ctx->nonce_len + len, ctx->tag_len)) return -1;
    } else if (ctx->algorithm == KCP_CRYPT_AES_GCM) {
        if (len) memmove(out + 12, data, len);
        if (kcp_crypto_random(ctx->backend, out, 12)) return -1;
        if (kcp_crypto_gcm(ctx->backend, out + 12, len, out, out + 12 + len, 1)) return -1;
    } else {
        if (len) memmove(out + LEGACY_HEADER, data, len);
        if (kcp_crypto_random(ctx->backend, out, 16)) return -1;
        crc = crc32_ieee(out + LEGACY_HEADER, len);
        for (i = 0; i < 4; ++i) out[16 + i] = (uint8_t)(crc >> (8 * i));
        if (kcp_crypto_transform(ctx->backend, out, len + LEGACY_HEADER, 1)) return -1;
    }
    *outlen = len + overhead;
    return 0;
}

/* Authenticate or check the CRC and decode one packet. */
int kcp_crypt_open(kcp_crypt *ctx, const uint8_t *packet, size_t len,
                   uint8_t *out, size_t capacity, size_t *outlen) {
    size_t overhead;
    uint32_t expected = 0;
    uint8_t plain[MAX_PACKET];
    int i, result;
    if (!ctx || !packet || !out || !outlen) { errno = EINVAL; return -1; }
    overhead = kcp_crypt_overhead(ctx);
    if (len < overhead || len > MAX_PACKET) return 1;
    if (capacity < len - overhead) { errno = EMSGSIZE; return -1; }
    /* Authenticate into a temporary buffer before releasing plaintext. */
    if (ctx->seal) {
        memcpy(plain, packet + ctx->nonce_len, len - overhead);
        result = ctx->open(ctx->user, packet, ctx->nonce_len, plain, len - overhead,
                           packet + len - ctx->tag_len, ctx->tag_len);
        if (result) return result;
        memcpy(out, plain, len - overhead);
    } else if (ctx->algorithm == KCP_CRYPT_AES_GCM) {
        memcpy(plain, packet + 12, len - overhead);
        result = kcp_crypto_gcm(ctx->backend, plain, len - overhead, packet,
                                (uint8_t *)(packet + len - 16), 0);
        if (result) return result;
        memcpy(out, plain, len - overhead);
    } else {
        memcpy(plain, packet, len);
        if (kcp_crypto_transform(ctx->backend, plain, len, 0)) return -1;
        for (i = 0; i < 4; ++i) expected |= (uint32_t)plain[16 + i] << (8 * i);
        if (expected != crc32_ieee(plain + LEGACY_HEADER, len - overhead)) return 1;
        memcpy(out, plain + LEGACY_HEADER, len - overhead);
    }
    *outlen = len - overhead;
    return 0;
}
