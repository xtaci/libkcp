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

#include "crypt_backend.h"
#include <tomcrypt.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#else
#include <pthread.h>
#endif

static const uint8_t iv[16] = {167, 115, 79, 156, 18, 172, 27, 1, 164, 21, 242, 193, 252, 120, 230, 107};
static int aes_index = -1, sha1_index = -1;

/* Cipher state and system entropy source for one packet-crypto context. */
struct backend {
    kcp_crypt_algorithm algorithm; /* Selected kcp-go wire format. */
    symmetric_key cipher;          /* Expanded LibTomCrypt block-cipher key. */
    uint32_t tea_key[4];           /* Big-endian TEA key words. */
    uint8_t key[1472];             /* GCM/Salsa key or expanded XOR table. */
    size_t keylen;                 /* AES-GCM key length. */
#ifndef _WIN32
    FILE *random;                  /* Open operating-system entropy device. */
#endif
};

/* Register the descriptors needed by LibTomCrypt's GCM and PBKDF2 APIs. */
static void register_algorithms(void) {
    aes_index = register_cipher(&aes_desc);
    sha1_index = register_hash(&sha1_desc);
}

#ifdef _WIN32
static INIT_ONCE crypto_once = INIT_ONCE_STATIC_INIT;
/* Initialize shared LibTomCrypt descriptors once across Windows threads. */
static BOOL CALLBACK initialize_crypto(PINIT_ONCE once, PVOID parameter, PVOID *context) {
    (void)once; (void)parameter; (void)context;
    register_algorithms();
    return TRUE;
}
/* Ensure shared descriptors are ready before creating a context. */
static int crypto_init(void) {
    if (!InitOnceExecuteOnce(&crypto_once, initialize_crypto, NULL, NULL)) { errno = EIO; return -1; }
    return 0;
}
#else
static pthread_once_t crypto_once = PTHREAD_ONCE_INIT;
/* Initialize shared descriptors once across POSIX threads. */
static int crypto_init(void) {
    if (pthread_once(&crypto_once, register_algorithms)) { errno = EIO; return -1; }
    return 0;
}
#endif

/* Read an unaligned big-endian TEA word. */
static uint32_t load_be32(const uint8_t *p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}
/* Write an unaligned big-endian TEA word. */
static void store_be32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
}
/* Read an unaligned little-endian Salsa20 word. */
static uint32_t load_le32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
/* Write an unaligned little-endian Salsa20 word. */
static void store_le32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}
/* Rotate a 32-bit Salsa20 word by a nonzero constant. */
static uint32_t rotate_left(uint32_t x, unsigned n) { return x << n | x >> (32 - n); }

/* kcp-go uses 16 TEA rounds, equivalent to eight two-word cycles. */
static void tea_encrypt(const uint32_t key[4], const uint8_t in[8], uint8_t out[8]) {
    uint32_t v0 = load_be32(in), v1 = load_be32(in + 4), sum = 0;
    int i;
    for (i = 0; i < 8; ++i) {
        sum += 0x9e3779b9u;
        v0 += ((v1 << 4) + key[0]) ^ (v1 + sum) ^ ((v1 >> 5) + key[1]);
        v1 += ((v0 << 4) + key[2]) ^ (v0 + sum) ^ ((v0 >> 5) + key[3]);
    }
    store_be32(out, v0); store_be32(out + 4, v1);
}

#define SALSA_QUARTER(a, b, c, d) do { \
    b ^= rotate_left(a + d, 7); c ^= rotate_left(b + a, 9); \
    d ^= rotate_left(c + b, 13); a ^= rotate_left(d + c, 18); \
} while (0)
/* XOR a packet with the Salsa20 stream starting at counter zero. */
static void salsa20_crypt(const uint8_t key[32], const uint8_t nonce[8], uint8_t *data, size_t len) {
    static const uint8_t sigma[16] = "expand 32-byte k";
    uint32_t state[16], work[16];
    uint8_t stream[64];
    size_t offset, count, j;
    int i;
    state[0] = load_le32(sigma); state[5] = load_le32(sigma + 4);
    state[10] = load_le32(sigma + 8); state[15] = load_le32(sigma + 12);
    for (i = 0; i < 4; ++i) {
        state[1 + i] = load_le32(key + 4 * i);
        state[11 + i] = load_le32(key + 16 + 4 * i);
    }
    state[6] = load_le32(nonce); state[7] = load_le32(nonce + 4);
    state[8] = state[9] = 0;
    for (offset = 0; offset < len; offset += count) {
        memcpy(work, state, sizeof(work));
        /* Ten double rounds alternate column and row quarter rounds. */
        for (i = 0; i < 10; ++i) {
            SALSA_QUARTER(work[0], work[4], work[8], work[12]);
            SALSA_QUARTER(work[5], work[9], work[13], work[1]);
            SALSA_QUARTER(work[10], work[14], work[2], work[6]);
            SALSA_QUARTER(work[15], work[3], work[7], work[11]);
            SALSA_QUARTER(work[0], work[1], work[2], work[3]);
            SALSA_QUARTER(work[5], work[6], work[7], work[4]);
            SALSA_QUARTER(work[10], work[11], work[8], work[9]);
            SALSA_QUARTER(work[15], work[12], work[13], work[14]);
        }
        for (i = 0; i < 16; ++i) store_le32(stream + 4 * i, work[i] + state[i]);
        count = len - offset < sizeof(stream) ? len - offset : sizeof(stream);
        for (j = 0; j < count; ++j) data[offset + j] ^= stream[j];
        if (++state[8] == 0) ++state[9];
    }
}
#undef SALSA_QUARTER

/* Prepare cipher-specific key state and an operating-system random source. */
void *kcp_crypto_create(kcp_crypt_algorithm algorithm, const uint8_t *key, size_t keylen) {
    static const uint8_t empty = 0;
    static const uint8_t salt[] = "sH3CIVoF#rWLtJo6";
    struct backend *b;
    unsigned long output_len;
    uint8_t repeated[14];
    size_t i, n;
    int result = CRYPT_OK;
    if (!key) key = &empty;
    if (keylen > ULONG_MAX) { errno = EINVAL; return NULL; }
    if (crypto_init()) return NULL;
    if (algorithm == KCP_CRYPT_SM4) { errno = ENOTSUP; return NULL; }
    b = (struct backend *)calloc(1, sizeof(*b));
    if (!b) return NULL;
    b->algorithm = algorithm;
#ifndef _WIN32
    b->random = fopen("/dev/urandom", "rb");
    if (!b->random) { free(b); return NULL; }
#endif
    /* Expand keys once; legacy CFB always uses the block encrypt primitive. */
    switch (algorithm) {
    case KCP_CRYPT_AES_GCM:
    case KCP_CRYPT_AES:
        if (keylen != 16 && keylen != 24 && keylen != 32) result = CRYPT_INVALID_KEYSIZE;
        else result = aes_setup(key, (int)keylen, 0, &b->cipher);
        if (algorithm == KCP_CRYPT_AES_GCM && result == CRYPT_OK) {
            memcpy(b->key, key, keylen); b->keylen = keylen;
            if (aes_index < 0) result = CRYPT_ERROR;
        }
        break;
    case KCP_CRYPT_TWOFISH:
        result = keylen == 16 || keylen == 24 || keylen == 32 ?
            twofish_setup(key, (int)keylen, 0, &b->cipher) : CRYPT_INVALID_KEYSIZE;
        break;
    case KCP_CRYPT_3DES: result = keylen == 24 ? des3_setup(key, 24, 0, &b->cipher) : CRYPT_INVALID_KEYSIZE; break;
    case KCP_CRYPT_CAST5: result = keylen == 16 ? cast5_setup(key, 16, 0, &b->cipher) : CRYPT_INVALID_KEYSIZE; break;
    case KCP_CRYPT_BLOWFISH:
        if (!keylen || keylen > 56) { result = CRYPT_INVALID_KEYSIZE; break; }
        if (keylen < 8) {
            /* Repeating an entire short key preserves Blowfish's cyclic schedule. */
            n = keylen;
            while (n < 8) n += keylen;
            for (i = 0; i < n; ++i) repeated[i] = key[i % keylen];
            result = blowfish_setup(repeated, (int)n, 0, &b->cipher);
        } else result = blowfish_setup(key, (int)keylen, 0, &b->cipher);
        break;
    case KCP_CRYPT_TEA:
        if (keylen != 16) result = CRYPT_INVALID_KEYSIZE;
        else for (i = 0; i < 4; ++i) b->tea_key[i] = load_be32(key + 4 * i);
        break;
    case KCP_CRYPT_XTEA: result = keylen == 16 ? xtea_setup(key, 16, 0, &b->cipher) : CRYPT_INVALID_KEYSIZE; break;
    case KCP_CRYPT_SALSA20:
        memcpy(b->key, key, keylen < 32 ? keylen : 32);
        break;
    case KCP_CRYPT_XOR:
        output_len = sizeof(b->key);
        if (sha1_index < 0) result = CRYPT_ERROR;
        else result = pkcs_5_alg2(key, (unsigned long)keylen, salt, sizeof(salt) - 1,
                                  32, sha1_index, b->key, &output_len);
        break;
    case KCP_CRYPT_NONE: break;
    default: result = CRYPT_INVALID_ARG;
    }
    if (result != CRYPT_OK) {
        kcp_crypto_destroy(b);
        errno = result == CRYPT_INVALID_KEYSIZE || result == CRYPT_INVALID_ARG ? EINVAL : EIO;
        return NULL;
    }
    return b;
}

/* Close the entropy source and erase sensitive cipher state. */
void kcp_crypto_destroy(void *ctx) {
    struct backend *b = (struct backend *)ctx;
    if (!b) return;
#ifndef _WIN32
    if (b->random) fclose(b->random);
#endif
    zeromem(b, sizeof(*b));
    free(b);
}

/* Obtain random bytes from the platform's cryptographic random source. */
int kcp_crypto_random(void *ctx, uint8_t *out, size_t len) {
    struct backend *b = (struct backend *)ctx;
    if (!len) return 0;
#ifdef _WIN32
    (void)b;
    if (len > ULONG_MAX || BCryptGenRandom(NULL, out, (ULONG)len, BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
        errno = EIO; return -1;
    }
#else
    if (fread(out, 1, len, b->random) != len) { errno = EIO; return -1; }
#endif
    return 0;
}

/* Apply kcp-go's legacy CFB, Salsa20, XOR or identity packet transform. */
int kcp_crypto_transform(void *ctx, uint8_t *data, size_t len, int encrypt) {
    struct backend *b = (struct backend *)ctx;
    uint8_t feedback[16], stream[16];
    size_t off, i, count, block = 0;
    int result;
    if (b->algorithm == KCP_CRYPT_SALSA20) {
        if (len < 8) { errno = EINVAL; return -1; }
        salsa20_crypt(b->key, data, data + 8, len - 8);
        return 0;
    }
    if (b->algorithm == KCP_CRYPT_XOR) {
        if (len > sizeof(b->key)) { errno = EMSGSIZE; return -1; }
        for (i = 0; i < len; ++i) data[i] ^= b->key[i];
        return 0;
    }
    if (b->algorithm == KCP_CRYPT_NONE) return 0;
    block = b->algorithm == KCP_CRYPT_AES || b->algorithm == KCP_CRYPT_TWOFISH ? 16 : 8;
    /* kcp-go resets CFB feedback to its fixed IV for every packet. */
    memcpy(feedback, iv, block);
    for (off = 0; off < len; off += count) {
        switch (b->algorithm) {
        case KCP_CRYPT_AES: result = aes_ecb_encrypt(feedback, stream, &b->cipher); break;
        case KCP_CRYPT_TWOFISH: result = twofish_ecb_encrypt(feedback, stream, &b->cipher); break;
        case KCP_CRYPT_3DES: result = des3_ecb_encrypt(feedback, stream, &b->cipher); break;
        case KCP_CRYPT_CAST5: result = cast5_ecb_encrypt(feedback, stream, &b->cipher); break;
        case KCP_CRYPT_BLOWFISH: result = blowfish_ecb_encrypt(feedback, stream, &b->cipher); break;
        case KCP_CRYPT_XTEA: result = xtea_ecb_encrypt(feedback, stream, &b->cipher); break;
        case KCP_CRYPT_TEA: tea_encrypt(b->tea_key, feedback, stream); result = CRYPT_OK; break;
        default: errno = EINVAL; return -1;
        }
        if (result != CRYPT_OK) { errno = EIO; return -1; }
        count = len - off < block ? len - off : block;
        for (i = 0; i < count; ++i) {
            uint8_t input = data[off + i];
            data[off + i] ^= stream[i];
            feedback[i] = encrypt ? data[off + i] : input;
        }
    }
    return 0;
}

/* Encrypt or authenticate a packet using AES-GCM. */
int kcp_crypto_gcm(void *ctx, uint8_t *data, size_t len, const uint8_t *nonce,
                   uint8_t *tag, int encrypt) {
    struct backend *b = (struct backend *)ctx;
    uint8_t calculated[16];
    unsigned long tag_len = sizeof(calculated);
    int result;
    if (len > ULONG_MAX) { errno = EMSGSIZE; return -1; }
    result = gcm_memory(aes_index, b->key, (unsigned long)b->keylen, nonce, 12,
                        NULL, 0, data, (unsigned long)len, data, calculated, &tag_len,
                        encrypt ? GCM_ENCRYPT : GCM_DECRYPT);
    if (result != CRYPT_OK || tag_len != sizeof(calculated)) { errno = EIO; return -1; }
    if (encrypt) { memcpy(tag, calculated, sizeof(calculated)); return 0; }
    /* Compare tags without data-dependent early exits. */
    return mem_neq(tag, calculated, sizeof(calculated)) ? 1 : 0;
}
