// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

using slot_id_t = int;

enum class net_event_type_t {
  rpc_answer,
  rpc_error,
};

struct net_event_t {
  net_event_type_t type;
  union {
    slot_id_t slot_id;
    slot_id_t rpc_id;
  };
  union {
    struct { //rpc_answer
      int result_len;
      //allocated via dl_malloc
      char *result;
    };
    struct { //rpc_error
      int error_code;
      const char *error_message;
    };
  };
};

enum net_query_type_t {
  nq_rpc_send
};

struct net_query_t {
  net_query_type_t type;
  slot_id_t slot_id;
  union {
    struct { //nq_rpc_send
      int host_num;
      char *request;
      int request_size;
      int timeout_ms;
    };
  };
};

#pragma pack(push, 4)

/***
 QUERY MEMORY ALLOCATOR
 ***/

void qmem_init();
void *qmem_malloc(size_t n);
void qmem_free_ptrs();
void qmem_clear();

extern long long qmem_generation;

const char *qmem_pstr(char const *msg, ...) __attribute__ ((format (printf, 1, 2)));

struct str_buf_t {
  int len, buf_len;
  char *buf;
};

struct chain_t {
  chain_t *next, *prev;
  char *buf;
  int len;
};

/***
  QUERIES
 ***/

#define PHPQ_X2 0x12340000
#define PHPQ_RPC_ANSWER 0x43210000
#define PHPQ_CONNECT 0x2f2d0000
#define PHPQ_NETQ 0x3d780000
#define PHPQ_WAIT 0x728a0000
#define PHPQ_HTTP_LOAD_POST 0x5ac20000
#define NETQ_PACKET 1234

#define PNETF_IMMEDIATE 16

struct php_query_base_t {
  int type;
  void *ans;
};

/** test x^2 query **/
struct php_query_x2_answer_t {
  char *comment;
  int x2;
};

struct php_query_x2_t {
  php_query_base_t base;

  int val;
};

php_query_x2_answer_t *php_query_x2(int x);

/** rpc answer query **/
struct php_query_rpc_answer {
  php_query_base_t base;

  const char *data;
  int data_len;
};

/** create connection query **/
struct php_query_connect_answer_t {
  int connection_id;
};

enum protocol_t {
  p_memcached,
  p_sql,
  p_rpc,
};

struct php_query_connect_t {
  php_query_base_t base;

  const char *host;
  int port;
  protocol_t protocol;
};

php_query_connect_answer_t *php_query_connect(const char *host, int port, protocol_t protocol);

/** load long http post query **/
struct php_query_http_load_post_answer_t {
  int loaded_bytes;
};

struct php_query_http_load_post_t {
  php_query_base_t base;

  char *buf;
  int min_len;
  int max_len;
};


/** net query **/
struct data_reader_t {
  int len, readed;
  void (*read)(data_reader_t *reader, void *dest);
  void *extra;
};

enum nq_state_t {
  nq_error,
  nq_ok
};

struct php_net_query_packet_answer_t {
  nq_state_t state;

  const char *desc;
  const char *res;
  int res_len;
  chain_t *chain;

  long long result_id;
};

struct php_net_query_packet_t {
  php_query_base_t base;

  int connection_id;
  const char *data;
  int data_len;
  long long data_id;

  double timeout;
  protocol_t protocol;
  int extra_type;
};

/** wait query **/
struct php_net_query_wait_t {
  php_query_base_t base;
  int timeout_ms;
};


//php_net_query_packet_answer_t *php_net_query_packet (int connection_id, const char *data, int data_len, int timeout_ms);

/*** net answer generator base ***/
enum ansgen_state_t {
  st_ansgen_done,
  st_ansgen_error,
  st_ansgen_wait
};

struct net_ansgen_t;

struct net_ansgen_func_t {
  void (*error)(net_ansgen_t *self, const char *);
  void (*timeout)(net_ansgen_t *self);
  void (*set_desc)(net_ansgen_t *self, const char *);
  void (*free)(net_ansgen_t *self);
};

struct net_ansgen_t {
  long long qmem_req_generation;
  ansgen_state_t state;
  php_net_query_packet_answer_t *ans;
  net_ansgen_func_t *func;
};

/*** memcached answer generator ***/
struct mc_ansgen_t;

struct mc_ansgen_func_t {
  void (*value)(mc_ansgen_t *self, data_reader_t *data);
  void (*end)(mc_ansgen_t *self);
  void (*xstored)(mc_ansgen_t *self, int is_stored);
  void (*other)(mc_ansgen_t *self, data_reader_t *data);
  void (*version)(mc_ansgen_t *self, data_reader_t *data);
  void (*set_query_type)(mc_ansgen_t *self, int query_type);
};

struct mc_ansgen_t {
  net_ansgen_t base;
  mc_ansgen_func_t *func;
};

enum mc_ansgen_packet_state_t {
  ap_any,
  ap_get,
  ap_store,
  ap_other,
  ap_err,
  ap_version
};//TODO ans?

struct mc_ansgen_packet_t {
  net_ansgen_t base;
  mc_ansgen_func_t *func;

  mc_ansgen_packet_state_t state;

  str_buf_t *str_buf;
};

mc_ansgen_t *mc_ansgen_packet_create();

/*** command ***/
struct command_t {
  void (*run)(command_t *command, void *data);
  void (*free)(command_t *command);
};


/*** sql answer generator ***/
struct sql_ansgen_t;

struct sql_ansgen_func_t {
  void (*set_writer)(sql_ansgen_t *self, command_t *writer);
  void (*ready)(sql_ansgen_t *self, void *data);
  void (*packet)(sql_ansgen_t *self, data_reader_t *reader);
  void (*done)(sql_ansgen_t *self);
};

struct sql_ansgen_t {
  net_ansgen_t base;
  sql_ansgen_func_t *func;
};


enum sql_ansgen_packet_state_t {
  sql_ap_init,
  sql_ap_wait_conn,
  sql_ap_wait_ans
};//TODO ans?

struct sql_ansgen_packet_t {
  net_ansgen_t base;
  sql_ansgen_func_t *func;

  sql_ansgen_packet_state_t state;

  command_t *writer;

  chain_t *chain;
};

sql_ansgen_t *sql_ansgen_packet_create();

/*** net_send generator ***/
struct net_send_ansgen_t;

struct net_send_ansgen_func_t {
  void (*set_writer)(net_send_ansgen_t *self, command_t *writer);
  void (*send_and_finish)(net_send_ansgen_t *self, void *data);
};

struct net_send_ansgen_t {
  net_ansgen_t base;
  net_send_ansgen_func_t *func;

//  net_send_ansgen_state_t state;
  command_t *writer;
  long long qres_id;
};


/*** rpc interface ***/
// Simple structure with union is used. 
// Mostly because it makes Events easily reusable.

struct php_query_wait_t {
  php_query_base_t base;
  int timeout_ms;
};


net_query_t *pop_net_query();
void free_net_query(net_query_t *query);

int create_rpc_error_event(slot_id_t slot_id, int error_code, const char *error_message, net_event_t **res);
int create_rpc_answer_event(slot_id_t slot_id, int len, net_event_t **res);
int net_events_empty();

void php_queries_start();
void php_queries_finish();

void init_drivers();

int mc_connect_to(const char *host_name, int port);
void mc_run_query(int host_num, const char *request, int request_len, int timeout_ms, int query_type, void (*callback)(const char *result, int result_len));
int db_proxy_connect();
void db_run_query(int host_num, const char *request, int request_len, int timeout_ms, void (*callback)(const char *result, int result_len));
void set_server_status(const char *status, int status_len);
void set_server_status_rpc(int port, long long actor_id, double start_time);
double get_net_time();
double get_script_time();
int get_net_queries_count();
int get_engine_uptime();
const char *get_engine_version();
int http_load_long_query(char *buf, int min_len, int max_len);
void http_set_result(const char *headers, int headers_len, const char *body, int body_len, int exit_code);
void rpc_answer(const char *res, int res_len);
void rpc_set_result(const char *body, int body_len, int exit_code);
void script_error();
void finish_script(int exit_code);
int rpc_connect_to(const char *host_name, int port);
slot_id_t rpc_send_query(int host_num, char *request, int request_len, int timeout_ms);
void wait_net_events(int timeout_ms);
net_event_t *pop_net_event();
int query_x2(int x);


#pragma pack(pop)
