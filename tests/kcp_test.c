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

#include "sess.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifdef KCP_ENABLE_CRYPTO
/* Deliberately insecure AEAD used only to test the custom callback framing. */
static void test_tag(const uint8_t *key, const uint8_t *nonce, const uint8_t *data,
                     size_t len, uint8_t tag[16]) {
    uint32_t hash = 2166136261u;
    size_t i;
    for (i = 0; i < 16; ++i) hash = (hash ^ key[i]) * 16777619u;
    for (i = 0; i < 12; ++i) hash = (hash ^ nonce[i]) * 16777619u;
    for (i = 0; i < len; ++i) hash = (hash ^ data[i]) * 16777619u;
    for (i = 0; i < 16; ++i) tag[i] = (uint8_t)(hash >> (8 * (i % 4)));
}

/* Seal the interoperability suite's deliberately insecure test AEAD. */
static int test_seal(void *user, const uint8_t *nonce, size_t nonce_len,
                     uint8_t *data, size_t len, uint8_t *tag, size_t tag_len) {
    const uint8_t *key = (const uint8_t *)user;
    size_t i;
    if (nonce_len != 12 || tag_len != 16) { errno = EINVAL; return -1; }
    for (i = 0; i < len; ++i) data[i] ^= key[i % 16] ^ nonce[i % 12];
    test_tag(key, nonce, data, len, tag);
    return 0;
}

/* Authenticate and open the interoperability suite's test AEAD. */
static int test_open(void *user, const uint8_t *nonce, size_t nonce_len,
                     uint8_t *data, size_t len, const uint8_t *tag, size_t tag_len) {
    const uint8_t *key = (const uint8_t *)user;
    uint8_t expected[16], diff = 0;
    size_t i;
    if (nonce_len != 12 || tag_len != 16) { errno = EINVAL; return -1; }
    test_tag(key, nonce, data, len, expected);
    for (i = 0; i < 16; ++i) diff |= expected[i] ^ tag[i];
    if (diff) return 1;
    for (i = 0; i < len; ++i) data[i] ^= key[i % 16] ^ nonce[i % 12];
    return 0;
}
#endif

/* Send deterministic bytes to the echo server and verify the full reply. */
int main(int argc, char **argv) {
    unsigned long port = argc > 1 ? strtoul(argv[1], NULL, 10) : 9999;
    size_t data_shards = argc > 2 ? strtoul(argv[2], NULL, 10) : 2;
    size_t parity_shards = argc > 3 ? strtoul(argv[3], NULL, 10) : 2;
    int algorithm = argc > 4 ? atoi(argv[4]) : -1;
    size_t keylen = argc > 5 ? strtoul(argv[5], NULL, 10) : 16;
    size_t message_size = argc > 6 ? strtoul(argv[6], NULL, 10) : 4096;
    uint8_t key[32];
    kcp_crypt *crypt = NULL;
    kcp_session *s = NULL;
    char *msg = NULL, *buf = NULL;
    size_t i, received = 0;
    uint32_t start;
    int result = 1;
    for (i = 0; i < sizeof(key); ++i) key[i] = (uint8_t)i;
    if (!port || port > 65535 || keylen > sizeof(key) || !message_size || message_size > 131072) return 2;
    msg = (char *)malloc(message_size);
    buf = (char *)malloc(message_size);
    if (!msg || !buf) { perror("malloc"); goto cleanup; }
#ifdef KCP_ENABLE_CRYPTO
    if (algorithm >= 0) {
        crypt = algorithm == 12 ? kcp_crypt_create_aead(12, 16, test_seal, test_open, key)
                                : kcp_crypt_create((kcp_crypt_algorithm)algorithm, key, keylen);
        if (!crypt) { perror("kcp_crypt_create"); goto cleanup; }
    }
#else
    if (algorithm >= 0) { fprintf(stderr, "crypto disabled\n"); result = 2; goto cleanup; }
#endif
    s = kcp_session_dial("127.0.0.1", (uint16_t)port, data_shards, parity_shards, crypt);
    if (!s) { perror("dial"); goto cleanup; }
    kcp_session_nodelay(s, 1, 20, 2, 1);
    kcp_session_wndsize(s, 128, 128);
    kcp_session_set_stream(s, 1);
    for (i = 0; i < message_size; ++i) msg[i] = (char)(i * 17);
    if (kcp_session_write(s, msg, message_size) != (ssize_t)message_size) { perror("write"); goto cleanup; }
    start = kcp_current_ms();
    while (received < message_size && kcp_current_ms() - start < 8000) {
        ssize_t n;
        if (kcp_session_update(s, kcp_current_ms())) { perror("update"); goto cleanup; }
        n = kcp_session_read(s, buf + received, message_size - received);
        if (n < 0) { perror("read"); goto cleanup; }
        received += (size_t)n;
#ifdef _WIN32
        Sleep(1);
#else
        usleep(1000);
#endif
    }
    if (received != message_size || memcmp(msg, buf, message_size)) {
        fprintf(stderr, "echo mismatch: %zu bytes\n", received); goto cleanup;
    }
    result = 0;
cleanup:
    kcp_session_destroy(s);
#ifdef KCP_ENABLE_CRYPTO
    kcp_crypt_destroy(crypt);
#endif
    free(msg); free(buf);
    return result;
}
