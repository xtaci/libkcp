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
#include "sess.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

/* Encrypt and tag a payload with the deliberately insecure test callback. */
static int mock_seal(void *user, const uint8_t *nonce, size_t nonce_len,
                     uint8_t *data, size_t len, uint8_t *tag, size_t tag_len) {
    size_t i;
    uint8_t sum = *(uint8_t *)user;
    for (i = 0; i < nonce_len; ++i) sum ^= nonce[i];
    for (i = 0; i < len; ++i) { data[i] ^= 0x5a; sum ^= data[i]; }
    memset(tag, sum, tag_len);
    return 0;
}

/* Authenticate and reverse the deliberately insecure test callback. */
static int mock_open(void *user, const uint8_t *nonce, size_t nonce_len,
                     uint8_t *data, size_t len, const uint8_t *tag, size_t tag_len) {
    size_t i;
    uint8_t sum = *(uint8_t *)user;
    for (i = 0; i < nonce_len; ++i) sum ^= nonce[i];
    for (i = 0; i < len; ++i) sum ^= data[i];
    for (i = 0; i < tag_len; ++i) if (tag[i] != sum) return 1;
    for (i = 0; i < len; ++i) data[i] ^= 0x5a;
    return 0;
}

/* Exercise packet framing, authentication failures and MTU bounds. */
int main(void) {
    uint8_t key[32], plain[1453], packet[1472], decoded[1472];
    size_t i, n, got, payload_len;
    int alg;
    for (i = 0; i < sizeof(key); ++i) key[i] = (uint8_t)i;
    for (i = 0; i < sizeof(plain); ++i) plain[i] = (uint8_t)(i * 17);
    for (alg = KCP_CRYPT_AES_GCM; alg <= KCP_CRYPT_NONE; ++alg) {
        size_t keylen = alg == KCP_CRYPT_3DES ? 24 : 16;
        kcp_crypt *ctx;
        if (alg == KCP_CRYPT_SM4) {
            errno = 0;
            assert(!kcp_crypt_create(KCP_CRYPT_SM4, key, keylen) && errno == ENOTSUP);
            continue;
        }
        ctx = kcp_crypt_create((kcp_crypt_algorithm)alg, key, keylen);
        assert(ctx);
        payload_len = sizeof(packet) - kcp_crypt_overhead(ctx);
        assert(kcp_crypt_seal(ctx, plain, payload_len + 1, packet, sizeof(packet), &n) == -1);
        memcpy(packet, plain, payload_len);
        assert(kcp_crypt_seal(ctx, packet, payload_len, packet, sizeof(packet), &n) == 0);
        assert(n == sizeof(packet));
        assert(kcp_crypt_open(ctx, packet, n, decoded, sizeof(decoded), &got) == 0);
        assert(got == payload_len && !memcmp(decoded, plain, got));
        if (alg != KCP_CRYPT_NONE) {
            kcp_crypt *wrong;
            key[0] ^= 0x80;
            wrong = kcp_crypt_create((kcp_crypt_algorithm)alg, key, keylen);
            key[0] ^= 0x80;
            assert(wrong);
            assert(kcp_crypt_open(wrong, packet, n, decoded, sizeof(decoded), &got) == 1);
            kcp_crypt_destroy(wrong);
        }
        packet[n - 1] ^= 1;
        assert(kcp_crypt_open(ctx, packet, n, decoded, sizeof(decoded), &got) == 1);
        assert(kcp_crypt_open(ctx, packet, 3, decoded, sizeof(decoded), &got) == 1);
        assert(kcp_crypt_seal(ctx, plain, payload_len, packet, 1, &n) == -1);
        kcp_crypt_destroy(ctx);
    }
    assert(!kcp_crypt_create(KCP_CRYPT_AES, key, 3));
    assert(!kcp_crypt_create(KCP_CRYPT_BLOWFISH, key, 0));
    {
        kcp_crypt *ctx = kcp_crypt_create_aead(7, 9, mock_seal, mock_open, key);
        assert(ctx && kcp_crypt_overhead(ctx) == 16);
        assert(!kcp_crypt_create_aead(7, 9, NULL, mock_open, key));
        assert(kcp_crypt_seal(ctx, plain, sizeof(plain), packet, sizeof(packet), &n) == 0);
        assert(kcp_crypt_open(ctx, packet, n, decoded, sizeof(decoded), &got) == 0);
        assert(got == sizeof(plain) && !memcmp(decoded, plain, got));
        packet[0] ^= 1;
        assert(kcp_crypt_open(ctx, packet, n, decoded, sizeof(decoded), &got) == 1);
        kcp_crypt_destroy(ctx);
    }
    {
        kcp_crypt *ctx = kcp_crypt_create(KCP_CRYPT_AES_GCM, key, 16);
        kcp_session *s = kcp_session_dial("127.0.0.1", 9, 2, 2, ctx);
        assert(ctx && s);
        assert(kcp_session_set_mtu(s, 1436) == 0);
        assert(kcp_session_set_mtu(s, 1437) == -1);
        assert(kcp_session_set_mtu(s, 49) == -1);
        kcp_session_destroy(s);
        kcp_crypt_destroy(ctx);
        ctx = kcp_crypt_create_aead(1440, 10, mock_seal, mock_open, key);
        assert(ctx);
        assert(!kcp_session_dial("127.0.0.1", 9, 2, 2, ctx));
        kcp_crypt_destroy(ctx);
    }
    return 0;
}
