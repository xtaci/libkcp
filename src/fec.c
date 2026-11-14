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
#include "reedsolomon.h"
#include "autotune.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* One buffered received shard, excluding its FEC header. */
struct fec_slot {
    uint32_t seq;  /* FEC wire sequence number. */
    uint32_t time; /* Arrival time in wrapping milliseconds. */
    uint16_t flag; /* Data or parity type. */
    size_t len;    /* Buffered shard size. */
    uint8_t *data; /* Owned shard payload. */
};

/* Sending parameters remain fixed while receive parameters auto-tune. */
struct fec {
    int data, parity, total;          /* Sending data, parity and total counts. */
    int tx_count, rx_count, rx_limit; /* Pending send and receive counts and receive limit. */
    int rx_data, rx_parity, rx_total; /* Current receive shard counts. */
    int should_tune;                 /* A received type conflicts with the current layout. */
    uint32_t next, paws;             /* Next send sequence and send wrap boundary. */
    uint32_t rx_paws;                /* Receive sequence wrap boundary. */
    reedsolomon *rs, *rx_rs;         /* Send codec and possibly distinct receive codec. */
    autotune tune;                   /* Recent receive types for detecting shard counts. */
    uint8_t **tx;                    /* Owned sending shard buffers. */
    size_t *tx_len;                  /* Unpadded sizes of data shards. */
    struct fec_slot *rx;             /* Received shards in arrival order. */
};

/* Decode little-endian wire integers without alignment assumptions. */
static uint16_t read16(const uint8_t *p) { return (uint16_t)(p[0] | (uint16_t)p[1] << 8); }
/* Decode a 32-bit little-endian wire integer. */
static uint32_t read32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
/* Encode little-endian wire integers without alignment assumptions. */
static void put16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
/* Encode a 32-bit little-endian wire integer. */
static void put32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

/* Allocate sending and initially matching receiving state. */
fec *fec_create(int data, int parity) {
    fec *f;
    if (data <= 0 || parity <= 0 || data > 255 || parity > 256 - data) {
        errno = EINVAL; return NULL;
    }
    f = (fec *)calloc(1, sizeof(*f));
    if (!f) return NULL;
    f->data = data; f->parity = parity; f->total = data + parity;
    f->rx_data = data; f->rx_parity = parity; f->rx_total = data + parity;
    f->rx_limit = 3 * f->total;
    f->paws = (0xffffffffu / (uint32_t)f->total - 1) * (uint32_t)f->total;
    f->rx_paws = (0xffffffffu / (uint32_t)f->rx_total) * (uint32_t)f->rx_total;
    /* Share the initial codec until receive auto-tuning needs a different one. */
    f->rs = rs_create(data, parity);
    f->rx_rs = f->rs;
    f->tx = (uint8_t **)calloc((size_t)f->total, sizeof(*f->tx));
    f->tx_len = (size_t *)calloc((size_t)data, sizeof(*f->tx_len));
    f->rx = (struct fec_slot *)calloc((size_t)f->rx_limit, sizeof(*f->rx));
    if (!f->rs || !f->tx || !f->tx_len || !f->rx) {
        fec_destroy(f); errno = ENOMEM; return NULL;
    }
    return f;
}

/* Release pending shards and both codec states. */
void fec_destroy(fec *f) {
    int i;
    if (!f) return;
    if (f->tx) for (i = 0; i < f->total; ++i) free(f->tx[i]);
    if (f->rx) for (i = 0; i < f->rx_count; ++i) free(f->rx[i].data);
    free(f->rx); free(f->tx); free(f->tx_len);
    if (f->rx_rs != f->rs) rs_destroy(f->rx_rs);
    rs_destroy(f->rs); free(f);
}

/* Emit one data shard and parity when its sending group is complete. */
int fec_encode(fec *f, const uint8_t *data, size_t len, fec_output_fn emit, void *user) {
    uint8_t packet[FEC_MAX_PACKET];
    size_t maxlen = 0;
    int i, ret = 0;
    if (!f || (!data && len) || !emit) { errno = EINVAL; return -1; }
    if (len > FEC_MAX_PACKET - FEC_DATA_HEADER_SIZE || len > UINT16_MAX - 2) {
        errno = EMSGSIZE; return -1;
    }
    /* Preserve the original length inside each shard before zero padding. */
    f->tx[f->tx_count] = (uint8_t *)malloc(len + 2);
    if (!f->tx[f->tx_count]) return -1;
    put16(f->tx[f->tx_count], (uint16_t)(len + 2));
    if (len) memcpy(f->tx[f->tx_count] + 2, data, len);
    f->tx_len[f->tx_count++] = len + 2;
    put32(packet, f->next++); put16(packet + 4, FEC_TYPE_DATA);
    memcpy(packet + 6, f->tx[f->tx_count - 1], len + 2);
    if (emit(user, packet, len + FEC_DATA_HEADER_SIZE)) ret = -1;
    if (f->tx_count != f->data) return ret;
    /* All Reed-Solomon inputs have the same size, including shorter packets. */
    for (i = 0; i < f->data; ++i)
        if (f->tx_len[i] > maxlen) maxlen = f->tx_len[i];
    for (i = 0; i < f->total; ++i) {
        uint8_t *p = (uint8_t *)realloc(f->tx[i], maxlen);
        if (!p) { ret = -1; goto clear; }
        f->tx[i] = p;
        if (i < f->data && f->tx_len[i] < maxlen)
            memset(p + f->tx_len[i], 0, maxlen - f->tx_len[i]);
    }
    if (rs_encode(f->rs, f->tx, maxlen)) { ret = -1; goto clear; }
    for (i = f->data; i < f->total; ++i) {
        put32(packet, f->next++); put16(packet + 4, FEC_TYPE_PARITY);
        memcpy(packet + 6, f->tx[i], maxlen);
        if (emit(user, packet, maxlen + 6)) ret = -1;
    }
    if (f->next >= f->paws) f->next = 0;
clear:
    /* Skip parity sequence numbers if encoding failed, keeping group boundaries. */
    if (f->next % (uint32_t)f->total)
        f->next += (uint32_t)f->total - f->next % (uint32_t)f->total;
    if (f->next >= f->paws) f->next = 0;
    for (i = 0; i < f->total; ++i) { free(f->tx[i]); f->tx[i] = NULL; }
    f->tx_count = 0;
    return ret;
}

/* Remove a shard while preserving arrival order for oldest-first eviction. */
static void remove_slot(fec *f, int index) {
    free(f->rx[index].data);
    --f->rx_count;
    if (index != f->rx_count)
        memmove(f->rx + index, f->rx + index + 1,
                (size_t)(f->rx_count - index) * sizeof(*f->rx));
}

/* Deliver data immediately and reconstruct missing shards when possible. */
int fec_input(fec *f, const uint8_t *packet, size_t len, uint32_t now,
              fec_output_fn emit, void *user) {
    uint32_t seq, group;
    uint16_t flag, payload_len;
    int i, count = 0, data_count = 0, result = 0;
    size_t maxlen = 0;
    uint8_t **shards, *present;
    if (!f || !packet || !emit) { errno = EINVAL; return -1; }
    if (len < FEC_DATA_HEADER_SIZE || len > FEC_MAX_PACKET) return 1;
    seq = read32(packet); flag = read16(packet + 4);
    if (flag != FEC_TYPE_DATA && flag != FEC_TYPE_PARITY) return 1;
    if (flag == FEC_TYPE_DATA) {
        payload_len = read16(packet + 6);
        if (payload_len < 2 || payload_len > len - 6) return 1;
    }
    /* Sample even duplicates; conflicting observations must not form a pulse. */
    autotune_add(&f->tune, flag == FEC_TYPE_DATA, seq);
    for (i = 0; i < f->rx_count;) {
        if (now - f->rx[i].time > 30000) remove_slot(f, i);
        else if (f->rx[i].seq == seq) return 0;
        else ++i;
    }
    /* Deliver data immediately, including while the decoder is learning. */
    if (flag == FEC_TYPE_DATA && emit(user, packet + 8, payload_len - 2)) return -1;
    if (seq >= f->rx_paws) return 0;
    if ((seq % (uint32_t)f->rx_total < (uint32_t)f->rx_data) != (flag == FEC_TYPE_DATA))
        f->should_tune = 1;
    if (f->should_tune) {
        int data = autotune_period(&f->tune, 1);
        int parity = autotune_period(&f->tune, 0);
        if (data > 0 && parity > 0 && data + parity <= 256) {
            if (data != f->rx_data || parity != f->rx_parity) {
                /* Allocate the new decoder before discarding the old receive state. */
                reedsolomon *codec = rs_create(data, parity);
                struct fec_slot *slots;
                int limit = 3 * (data + parity);
                if (!codec) return -1;
                slots = (struct fec_slot *)calloc((size_t)limit, sizeof(*slots));
                if (!slots) { rs_destroy(codec); return -1; }
                for (i = 0; i < f->rx_count; ++i) free(f->rx[i].data);
                free(f->rx);
                if (f->rx_rs != f->rs) rs_destroy(f->rx_rs);
                f->rx_rs = codec; f->rx = slots; f->rx_count = 0; f->rx_limit = limit;
                f->rx_data = data; f->rx_parity = parity; f->rx_total = data + parity;
                f->rx_paws = (0xffffffffu / (uint32_t)f->rx_total) * (uint32_t)f->rx_total;
            }
            f->should_tune = 0;
        }
        return 0;
    }
    if (f->rx_count == f->rx_limit) remove_slot(f, 0);
    {
        struct fec_slot *slot = &f->rx[f->rx_count];
        slot->data = (uint8_t *)malloc(len - 6);
        if (!slot->data) return -1;
        memcpy(slot->data, packet + 6, len - 6);
        slot->len = len - 6; slot->seq = seq; slot->flag = flag; slot->time = now;
        ++f->rx_count;
    }
    /* Gather the current group, reconstructing only once enough shards exist. */
    group = seq - seq % (uint32_t)f->rx_total;
    for (i = 0; i < f->rx_count; ++i) {
        if (f->rx[i].seq - f->rx[i].seq % (uint32_t)f->rx_total == group) {
            ++count; data_count += f->rx[i].flag == FEC_TYPE_DATA;
            if (f->rx[i].len > maxlen) maxlen = f->rx[i].len;
        }
    }
    if (count < f->rx_data) return 0;
    if (data_count == f->rx_data) goto remove_group;
    shards = (uint8_t **)calloc((size_t)f->rx_total, sizeof(*shards));
    present = (uint8_t *)calloc((size_t)f->rx_total, 1);
    if (!shards || !present) { free(shards); free(present); return -1; }
    for (i = 0; i < f->rx_total; ++i) {
        shards[i] = (uint8_t *)calloc(maxlen, 1);
        if (!shards[i]) { result = -1; goto cleanup; }
    }
    for (i = 0; i < f->rx_count; ++i) {
        struct fec_slot *slot = &f->rx[i];
        if (slot->seq - slot->seq % (uint32_t)f->rx_total == group) {
            int index = (int)(slot->seq % (uint32_t)f->rx_total);
            memcpy(shards[index], slot->data, slot->len);
            present[index] = 1;
        }
    }
    if (rs_reconstruct(f->rx_rs, shards, present, maxlen)) { result = -1; goto cleanup; }
    /* Forward only recovered data; original data was already delivered. */
    for (i = 0; i < f->rx_data; ++i) {
        if (!present[i]) {
            payload_len = read16(shards[i]);
            if (payload_len < 2 || payload_len > maxlen) { result = 1; break; }
            if (emit(user, shards[i] + 2, payload_len - 2)) { result = -1; break; }
        }
    }
cleanup:
    for (i = 0; i < f->rx_total; ++i) free(shards[i]);
    free(shards); free(present);
remove_group:
    for (i = 0; i < f->rx_count;) {
        if (f->rx[i].seq - f->rx[i].seq % (uint32_t)f->rx_total == group) remove_slot(f, i);
        else ++i;
    }
    return result;
}
