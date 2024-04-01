// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __NET_MYSQL_CLIENT__
#define __NET_MYSQL_CLIENT__

#include <sys/cdefs.h>

#include "common/mysql.h"

#include "net/net-connections.h"

struct mysql_client_functions {
  void *info;
  int server_charset;
  int (*execute)(struct connection *c, int op); /* invoked from parse_execute() */
  int (*sql_authorized)(struct connection *c);
  int (*sql_becomes_ready)(struct connection *c);
  int (*sql_wakeup)(struct connection *c);
  int (*check_ready)(struct connection *c);                      /* invoked from mc_client_check_ready() */
  int (*sql_flush_packet)(struct connection *c, int packet_len); /* execute this to push query to server */
  int (*sql_ready_to_write)(struct connection *c);
  int (*sql_check_perm)(struct connection *c);                                 /* 1 = allow unencrypted, 2 = allow encrypted */
  int (*sql_init_crypto)(struct connection *c, char *init_buff, int init_len); /* >0 = ok, bytes written; -1 = no crypto */
  const char *(*sql_get_database)(struct connection *c);
  const char *(*sql_get_user)(struct connection *c);
  const char *(*sql_get_password)(struct connection *c);
  bool (*is_mysql_same_datacenter_check_disabled)();
};

enum sql_response_state {
  resp_first, /* waiting for the first packet */
  resp_reading_fields,
  resp_reading_rows,
  resp_done
};

/* in conn->custom_data */
struct sqlc_data {
  int auth_state;
  int auth_user;
  int packet_state;
  int packet_len;
  int packet_seq;
  int response_state;
  int client_flags;
  int max_packet_size;
  int table_offset;
  int table_len;
  int server_capabilities;
  int server_language;
  int clen;
  int packet_padding;
  int block_size;
  int crypto_flags;
  char comm[16];
  char version[8];
  int extra_flags;
};

#define SQLC_DATA(c) ((struct sqlc_data *)((c)->custom_data))
#define SQLC_FUNC(c) ((struct mysql_client_functions *)((c)->extra))

extern conn_type_t ct_mysql_client;
extern struct mysql_client_functions default_mysql_client;

int sqlc_parse_execute(struct connection *c);
int sqlc_do_wakeup(struct connection *c);
int sqlc_execute(struct connection *c, int op);
int sqlc_init_outbound(struct connection *c);
int sqlc_password(struct connection *c, const char *user, char buffer[20]);

int sqlc_flush_packet(struct connection *c, int packet_len);
int sqlc_default_check_perm(struct connection *c);
int sqlc_init_crypto(struct connection *c, char *init_buff, int init_len);

/* END */
#endif
