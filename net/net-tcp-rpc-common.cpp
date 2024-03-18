// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "net/net-tcp-rpc-common.h"

#include <assert.h>
#include <stdio.h>
#include <sys/uio.h>

#include "common/kprintf.h"
#include "common/options.h"
#include "common/tl/constants/common.h"

#include "net/net-msg.h"
#include "net/net-tcp-connections.h"

int default_rpc_flags = RPC_CRYPTO_USE_CRC32C;

OPTION_PARSER(OPT_RPC, "no-crc32c", no_argument, "Force use of CRC32 instead of CRC32-C for TCP RPC protocol") {
  default_rpc_flags &= ~RPC_CRYPTO_USE_CRC32C;
  return 0;
}

// Flags:
//   Flag 1 - can not edit this message. Need to make copy.

void tcp_rpc_conn_send(struct connection *c, raw_message_t *raw, int flags) {
  tvkprintf(net_connections, 4, "%s: sending message of size %d to conn fd=%d\n", __func__, raw->total_bytes, c->fd);
  assert(!(raw->total_bytes & 3));
  int Q[2];
  Q[0] = raw->total_bytes + 12;
  Q[1] = TCP_RPC_DATA(c)->out_packet_num++;
  raw_message_t r;
  if (flags & 1) {
    rwm_clone(&r, raw);
  } else {
    r = *raw;
  }
  rwm_push_data_front(&r, Q, 8);
  unsigned crc32 = rwm_custom_crc32(&r, r.total_bytes, TCP_RPC_DATA(c)->custom_crc_partial);
  rwm_push_data(&r, &crc32, 4);
  rwm_union(&c->out, &r);
}

void tcp_rpc_conn_send_data(struct connection *c, int len, void *Q) {
  assert(!(len & 3));
  raw_message_t r;
  assert(rwm_create(&r, Q, len) == len);
  tcp_rpc_conn_send(c, &r, 0);
}

void net_rpc_send_ping(struct connection *c, long long ping_id) {
  tvkprintf(net_connections, 4, "Sending ping to fd=%d. ping_id = %lld\n", c->fd, ping_id);
  assert(c->flags & C_RAWMSG);
  static int P[20];
  P[0] = TL_RPC_PING;
  *(long long *)(P + 1) = ping_id;
  tcp_rpc_conn_send_data(c, 12, P);
  flush_later(c);
}
