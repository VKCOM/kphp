// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __NET_MEMCACHE_CLIENT__
#define __NET_MEMCACHE_CLIENT__

#include <sys/cdefs.h>

#include "net/net-connections.h"

struct memcache_client_functions {
  void *info;
  int (*execute)(struct connection *c, int op);                         /* invoked from parse_execute() */
  int (*check_ready)(struct connection *c);                             /* invoked from mc_client_check_ready() */
  int (*flush_query)(struct connection *c);                             /* execute this to push query to server */
  int (*connected)(struct connection *c);                               /* arseny30: invoked from mcc_connected() */
  int (*mc_check_perm)(struct connection *c);                           /* 1 = allow unencrypted, 2 = allow encrypted */
  int (*mc_init_crypto)(struct connection *c);                          /* 1 = ok, DELETE sent; -1 = no crypto */
  int (*mc_start_crypto)(struct connection *c, char *key, int key_len); /* 1 = ok, DELETE sent; -1 = no crypto */
};

extern conn_type_t ct_memcache_client;
extern struct memcache_client_functions default_memcache_client;

/* in conn->custom_data */
struct mcc_data {
  int response_len;
  int response_type;
  int response_flags;
  int key_offset;
  int key_len;
  int arg_num;
  long long args[4];
  int clen;
  char comm[16];
  char nonce[16];
  int nonce_time;
  int crypto_flags; /* 1 = allow unencrypted, 2 = allow encrypted, 4 = DELETE sent, waiting for NONCE/NOT_FOUND, 8 = encryption ON */
};

/* for mcc_data.response_type */
enum mc_response_type {
  mcrt_none,
  mcrt_empty,
  mcrt_VALUE,
  mcrt_NONCE,
  mcrt_NUMBER,
  mcrt_VERSION,
  mcrt_CLIENT_ERROR,
  mcrt_SERVER_ERROR,
  mcrt_STORED,
  mcrt_NOTSTORED,
  mcrt_DELETED,
  mcrt_NOTFOUND,
  mcrt_ERROR,
  mcrt_END
};

#define MCC_DATA(c) ((struct mcc_data *)((c)->custom_data))
#define MCC_FUNC(c) ((struct memcache_client_functions *)((c)->extra))

int mcc_execute(struct connection *c, int op);
int mc_client_check_ready(struct connection *c);
int mcc_init_outbound(struct connection *c);

int mcc_flush_query(struct connection *c);
int mcc_default_check_perm(struct connection *c);
int mcc_init_crypto(struct connection *c);
int mcc_start_crypto(struct connection *c, char *key, int key_len);

/* END */
#endif
