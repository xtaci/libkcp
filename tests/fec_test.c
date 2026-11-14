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

#include "fec.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Small set of captured FEC packets and their lengths. */
struct packets {
    uint8_t data[8][2048]; /* Packet contents. */
    size_t len[8];         /* Length of each captured packet. */
    int count;             /* Number of captured packets. */
};
/* Copy a callback packet into the bounded test capture. */
static int save(void *user, const uint8_t *data, size_t len) {
    struct packets *p = (struct packets *)user;
    assert(p->count < 8 && len <= sizeof(p->data[0]));
    memcpy(p->data[p->count], data, len);
    p->len[p->count++] = len;
    return 0;
}

/* Test receive reconfiguration, recovery and independent send parameters. */
static void tune_and_recover(void) {
    fec *rx = fec_create(2, 2), *tx = fec_create(5, 3);
    struct packets wire, received;
    int group, i;
    assert(rx && tx);
    for (group = 0; group < 5; ++group) {
        memset(&wire, 0, sizeof(wire));
        memset(&received, 0, sizeof(received));
        for (i = 0; i < 5; ++i) {
            uint8_t msg = (uint8_t)(group * 5 + i);
            assert(!fec_encode(tx, &msg, 1, save, &wire));
        }
        assert(wire.count == 8);
        for (i = 0; i < wire.count; ++i)
            if (group != 4 || i >= 3)
                assert(!fec_input(rx, wire.data[i], wire.len[i], 100, save, &received));
        assert(received.count == 5);
        if (group == 4) {
            for (i = 0; i < 3; ++i)
                assert(received.len[i + 2] == 1 && received.data[i + 2][0] == 20 + i);
        }
    }
    memset(&wire, 0, sizeof(wire));
    assert(!fec_encode(rx, (const uint8_t *)"a", 1, save, &wire));
    assert(!fec_encode(rx, (const uint8_t *)"b", 1, save, &wire));
    assert(wire.count == 4);
    fec_destroy(tx);
    tx = fec_create(3, 2);
    assert(tx);
    for (group = 0; group < 75; ++group) {
        memset(&wire, 0, sizeof(wire));
        memset(&received, 0, sizeof(received));
        for (i = 0; i < 3; ++i) {
            uint8_t msg = (uint8_t)(group * 3 + i);
            assert(!fec_encode(tx, &msg, 1, save, &wire));
        }
        for (i = 0; i < wire.count; ++i) {
            uint32_t seq = (uint32_t)(45 + group * 5 + i);
            wire.data[i][0] = (uint8_t)seq;
            wire.data[i][1] = (uint8_t)(seq >> 8);
            wire.data[i][2] = (uint8_t)(seq >> 16);
            wire.data[i][3] = (uint8_t)(seq >> 24);
            if (group != 74 || i >= 2)
                assert(!fec_input(rx, wire.data[i], wire.len[i], 100, save, &received));
        }
        if (received.count < 3) fprintf(stderr, "reconfiguration group %d: received %d\n", group, received.count);
        assert(received.count >= 3);
        if (group == 74) {
            assert(received.count == 3);
            assert(received.data[1][0] == (uint8_t)(group * 3));
            assert(received.data[2][0] == (uint8_t)(group * 3 + 1));
        }
    }
    fec_destroy(tx); fec_destroy(rx);
}

/* A full 256-shard group of small test packets. */
struct large_packets {
    uint8_t data[256][16]; /* Packet contents. */
    size_t len[256];       /* Length of each packet. */
    int count;             /* Number of captured packets. */
};
/* Capture a packet from a maximum-size shard group. */
static int save_large(void *user, const uint8_t *data, size_t len) {
    struct large_packets *p = (struct large_packets *)user;
    assert(p->count < 256 && len <= sizeof(p->data[0]));
    memcpy(p->data[p->count], data, len);
    p->len[p->count++] = len;
    return 0;
}

/* Count payload callbacks while verifying their byte value. */
struct count_payloads {
    int count;        /* Number of delivered payloads. */
    uint8_t expected; /* Byte expected in every test payload. */
};
/* Validate and count a delivered or recovered one-byte payload. */
static int count_payload(void *user, const uint8_t *data, size_t len) {
    struct count_payloads *p = (struct count_payloads *)user;
    assert(len == 1 && data[0] == p->expected);
    ++p->count;
    return 0;
}

/* Verify auto-tuning and recovery at the 256-shard boundary. */
static void tune_max_shards(int data, int parity) {
    fec *rx = fec_create(1, 1), *tx = fec_create(data, parity);
    struct large_packets wire;
    struct count_payloads received = {0, 0x42};
    int group, i, before;
    assert(rx && tx);
    for (group = 0; group < 3; ++group) {
        memset(&wire, 0, sizeof(wire));
        for (i = 0; i < data; ++i)
            assert(!fec_encode(tx, &received.expected, 1, save_large, &wire));
        assert(wire.count == 256);
        before = received.count;
        for (i = group == 2 ? 1 : 0; i < wire.count; ++i) {
            assert(!fec_input(rx, wire.data[i], wire.len[i], 100, count_payload, &received));
            if (group == 2 && received.count - before == data) break;
        }
        if (group == 2) assert(received.count - before == data);
    }
    fec_destroy(tx); fec_destroy(rx);
}

/* Exercise FEC recovery, configuration changes and parity vectors. */
int main(void) {
    fec *tx = fec_create(3, 2), *rx = fec_create(3, 2);
    struct packets wire = {{{0}}, {0}, 0}, recovered = {{{0}}, {0}, 0};
    const uint8_t msg0[] = "zero", msg1[] = "first packet", msg2[] = "second";
    assert(tx && rx);
    assert(fec_encode(tx, msg0, sizeof(msg0), save, &wire) == 0);
    assert(fec_encode(tx, msg1, sizeof(msg1), save, &wire) == 0);
    assert(fec_encode(tx, msg2, sizeof(msg2), save, &wire) == 0);
    assert(wire.count == 5);
    assert(fec_input(rx, wire.data[1], wire.len[1], 100, save, &recovered) == 0);
    assert(fec_input(rx, wire.data[1], wire.len[1], 100, save, &recovered) == 0);
    assert(fec_input(rx, wire.data[3], wire.len[3], 100, save, &recovered) == 0);
    assert(fec_input(rx, wire.data[4], wire.len[4], 100, save, &recovered) == 0);
    assert(recovered.count == 3);
    assert(recovered.len[0] == sizeof(msg1) && !memcmp(recovered.data[0], msg1, sizeof(msg1)));
    assert(recovered.len[1] == sizeof(msg0) && !memcmp(recovered.data[1], msg0, sizeof(msg0)));
    assert(recovered.len[2] == sizeof(msg2) && !memcmp(recovered.data[2], msg2, sizeof(msg2)));
    assert(fec_input(rx, wire.data[0], 5, 100, save, &recovered) == 1);
    fec_destroy(tx); fec_destroy(rx);
    tune_and_recover();
    tune_max_shards(1, 255);
    tune_max_shards(255, 1);
    {
        static const uint8_t expected3[] = {7, 0, 40, 41, 80, 52};
        static const uint8_t expected4[] = {7, 0, 84, 71, 161, 184};
        struct packets vectors = {{{0}}, {0}, 0};
        tx = fec_create(3, 2);
        assert(tx);
        assert(!fec_encode(tx, (const uint8_t *)"abc", 3, save, &vectors));
        assert(!fec_encode(tx, (const uint8_t *)"xy", 2, save, &vectors));
        assert(!fec_encode(tx, (const uint8_t *)"1234", 4, save, &vectors));
        assert(vectors.len[3] == 12 && vectors.len[4] == 12);
        assert(!memcmp(vectors.data[3] + 6, expected3, sizeof(expected3)));
        assert(!memcmp(vectors.data[4] + 6, expected4, sizeof(expected4)));
        fec_destroy(tx);
    }
    return 0;
}
