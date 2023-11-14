// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <optional>

#include "common/sanitizer.h"

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

void server_rpc_error(connection *c, long long req_id, int code, const char *str);

void http_return(connection *c, const char *str, int len);

/** delayed httq queries queue **/
extern connection pending_http_queue;
extern int php_worker_run_flag;

struct conn_query_functions;
extern conn_query_functions pending_cq_func;

extern command_t command_net_write_rpc_base;

extern conn_target_t rpc_ct;

void send_rpc_query(connection *c, int op, long long id, int *q, int qsize) ubsan_supp("alignment");
void on_net_event(int event_status);
void create_delayed_send_query(conn_target_t *t, command_t *command, double finish_time);

int get_target(const char *host, int port, conn_target_t *ct);

void net_error(net_ansgen_t *ansgen, php_query_base_t *query, const char *err);
conn_query *create_pnet_query(connection *http_conn, connection *conn, net_ansgen_t *gen, double finish_time);
void pnet_query_delete(conn_query *q);

extern int run_once_count;
extern int queries_to_recreate_script;

void turn_sigterm_on();

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

int delete_pending_query(conn_query *q);

