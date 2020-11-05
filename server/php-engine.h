// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "server/php-queries.h"

struct conn_query;
struct connection;
typedef struct conn_target conn_target_t;

enum class php_mode {
  cli,
  server
};

int run_main(int argc, char **argv, php_mode mode);

struct command_net_write_t {
  command_t base;

  unsigned char *data;
  int len;
  long long extra;
};

void net_error(net_ansgen_t *ansgen, php_query_base_t *query, const char *err);
conn_query *create_pnet_query(connection *http_conn, connection *conn, net_ansgen_t *gen, double finish_time);
void pnet_query_delete(conn_query *q);
extern void *php_script;
connection *get_target_connection(conn_target_t *S, int force_flag);
double fix_timeout(double timeout);
int pnet_query_check(conn_query *q);
data_reader_t *create_data_reader(connection *c, int data_len);
void create_pnet_delayed_query(connection *http_conn, conn_target_t *t, net_ansgen_t *gen, double finish_time);
void command_net_write_free(command_t *base_command);
command_t *create_command_net_writer(const char *data, int data_len, command_t *base, long long extra);
connection *get_target_connection_force(conn_target_t *S);
int pnet_query_timeout(conn_query *q);
void reopen_json_log();
