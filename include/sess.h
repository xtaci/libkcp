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

#ifndef KCP_SESS_H
#define KCP_SESS_H
#include <stddef.h>
#include <stdint.h>
#if defined(_WIN32) && defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#else
#include <sys/types.h>
#endif
#include "crypt.h"

#ifdef __cplusplus
extern "C" {
#endif
/* One connected, frame-driven KCP session. */
typedef struct kcp_session kcp_session;

/* Numeric IPv4 or IPv6 address. Both shard counts must be nonzero to enable FEC
 * sending; receiving automatically detects FEC. crypt is borrowed and must
 * outlive the session; NULL disables encryption. */
kcp_session *kcp_session_dial(const char *ip, uint16_t port, size_t data_shards,
                              size_t parity_shards, kcp_crypt *crypt);
/* Close the socket and release the session without freeing borrowed crypt. */
void kcp_session_destroy(kcp_session *session);
/* Nonblocking; returns -1 on socket, allocation or encryption failure. */
int kcp_session_update(kcp_session *session, uint32_t current_ms);
/* Read available bytes, returning zero if no data is ready and -1 on error. */
ssize_t kcp_session_read(kcp_session *session, char *buf, size_t size);
/* Queue bytes for the next update; returns the byte count or a negative error. */
ssize_t kcp_session_write(kcp_session *session, const char *buf, size_t size);
/* Set the six-bit IP differentiated services code point. */
int kcp_session_set_dscp(kcp_session *session, int dscp);
/* Enable or disable KCP stream mode. */
void kcp_session_set_stream(kcp_session *session, int enabled);
/* Configure KCP latency, timer, fast retransmit and congestion control. */
int kcp_session_nodelay(kcp_session *session, int nodelay, int interval, int resend, int nc);
/* Set KCP's send and receive windows in segments. */
int kcp_session_wndsize(kcp_session *session, int send_window, int receive_window);
/* mtu is the KCP payload MTU, excluding FEC and encryption overhead. */
int kcp_session_set_mtu(kcp_session *session, int mtu);
/* Return a wrapping millisecond clock suitable for session updates. */
uint32_t kcp_current_ms(void);
#ifdef __cplusplus
}
#endif
#endif
