// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __NET_TCP_RPC_CLIENT_H__
#define __NET_TCP_RPC_CLIENT_H__

#include <sys/cdefs.h>

#include "net/net-connections.h"
#include "net/net-tcp-rpc-common.h"
#include "net/net-msg.h"

struct tcp_rpc_client_functions {
  void *info;
  int (*execute)(struct connection *c, int op, struct raw_message *raw);	/* invoked from parse_execute() */
  int (*check_ready)(struct connection *c);		/* invoked from rpc_client_check_ready() */
  int (*flush_packet)(struct connection *c);		/* execute this to push query to server */
  int (*rpc_check_perm)(struct connection *c);		/* 1 = allow unencrypted, 2 = allow encrypted */
  int (*rpc_init_crypto)(struct connection *c);  	/* 1 = ok; -1 = no crypto */
  int (*rpc_start_crypto)(struct connection *c, struct tcp_rpc_nonce_packet *P);  /* 1 = ok; -1 = no crypto */
  int (*rpc_wakeup)(struct connection *c);
  int (*rpc_alarm)(struct connection *c);
  int (*rpc_ready)(struct connection *c);
  int (*rpc_close)(struct connection *c, int who);
  int max_packet_len, mode_flags;
};
extern conn_type_t ct_tcp_rpc_client;
conn_type_t get_default_tcp_rpc_client_conn_type();
int tcp_rpcc_parse_execute (struct connection *c);
int tcp_rpcc_connected (struct connection *c);
int tcp_rpcc_close_connection (struct connection *c, int who);
int tcp_rpcc_init_outbound (struct connection *c);
int tcp_rpc_client_check_ready (struct connection *c);
void tcp_rpcc_flush_crypto (struct connection *c);
int tcp_rpcc_flush (struct connection *c);
int tcp_rpcc_flush_packet (struct connection *c);
int tcp_rpcc_flush_packet_later (struct connection *c);
int tcp_rpcc_default_check_perm (struct connection *c);
int tcp_rpcc_default_check_perm_crypto (struct connection *c);
int tcp_rpcc_init_crypto (struct connection *c);
int tcp_rpcc_start_crypto (struct connection *c, struct tcp_rpc_nonce_packet *P);
int default_tcp_rpc_client_check_ready(struct connection *c);

#define TCP_RPCC_FUNC(c) ((struct tcp_rpc_client_functions *) ((c)->extra))

#endif
