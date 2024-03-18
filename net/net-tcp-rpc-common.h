// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __NET_TCP_RPC_COMMON_H__
#define __NET_TCP_RPC_COMMON_H__

#include <sys/cdefs.h>

#include "common/pid.h"
#include "common/sanitizer.h"

#include "net/net-connections.h"
#include "net/net-msg.h"

#pragma pack(push, 4)
struct tcp_rpc_nonce_packet {
  int type;
  int key_select;    /* least significant 32 bits of key to use */
  int crypto_schema; /* 0 = NONE, 1 = AES */
  int crypto_ts;
  char crypto_nonce[16];
};

struct tcp_rpc_handshake_packet {
  int type;
  int flags;
  struct process_id sender_pid;
  struct process_id peer_pid;
  /* more ints? */
};

struct tcp_rpc_handshake_error_packet {
  int type;
  int error_code;
  struct process_id sender_pid;
};
#pragma pack(pop)

void tcp_rpc_conn_send(struct connection *c, struct raw_message *raw, int flags);
void tcp_rpc_conn_send_data(struct connection *c, int len, void *Q);

/* in conn->custom_data */
struct tcp_rpc_data {
  int packet_len;
  int packet_num;
  int packet_type;
  int packet_crc32;
  int flags;
  int in_packet_num;
  int out_packet_num;
  int crypto_flags; /* 1 = allow unencrypted, 2 = allow encrypted, 4 = DELETE sent, waiting for NONCE/NOT_FOUND, 8 = encryption ON, 256 = packet numbers not
                       sequential, 512 = allow quick ack packets, 1024 = compact mode off */
  struct process_id remote_pid;
  char nonce[16];
  int nonce_time;
  union {
    void *user_data;
    void *extra;
  };
  int extra_int;
  int extra_int2;
  int extra_int3;
  int extra_int4;
  double extra_double, extra_double2;
  crc32_partial_func_t custom_crc_partial;
};

#define TCP_RPC_DATA(c) ((struct tcp_rpc_data *)((c)->custom_data))

#define RPC_CRYPTO_ALLOW_UNENCRYPTED 0x00000001
#define RPC_CRYPTO_ALLOW_ENCRYPTED 0x00000002
#define RPC_CRYPTO_ALLOW_MASK (RPC_CRYPTO_ALLOW_UNENCRYPTED | RPC_CRYPTO_ALLOW_ENCRYPTED)
#define RPC_CRYPTO_NONCE_SENT 0x00000004
#define RPC_CRYPTO_ENCRYPTION_ON 0x00000008
#define RPC_CRYPTO_ENCRYPTED_MASK (RPC_CRYPTO_ALLOW_ENCRYPTED | RPC_CRYPTO_NONCE_SENT | RPC_CRYPTO_ENCRYPTION_ON)
#define RPC_CRYPTO_ALLOW_QACK 0x00000200
#define RPC_CRYPTO_USE_CRC32C 0x00000800

#define RPC_NONCE 0x7acb87aa
#define RPC_HANDSHAKE 0x7682eef5
#define RPC_HANDSHAKE_ERROR 0x6a27beda

#define RPC_CRYPTO_NONE 0
#define RPC_CRYPTO_AES 1
#define RPC_CRYPTO_NONE_OR_AES 2

extern int default_rpc_flags; /* 0 = compatibility mode, RPC_CRYPTO_USE_CRC32C = allow both CRC32C and CRC32 */

void net_rpc_send_ping(struct connection *c, long long ping_id) ubsan_supp("alignment");

#endif
