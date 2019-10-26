#pragma once

/** http_query_data **/
struct http_query_data {
  char *uri, *get, *headers, *post, *request_method;
  int uri_len, get_len, headers_len, post_len, request_method_len;
  int keep_alive;
  unsigned int ip;
  unsigned int port;
};

http_query_data *http_query_data_create(const char *qUri, int qUriLen, const char *qGet, int qGetLen, const char *qHeaders,
                                        int qHeadersLen, const char *qPost, int qPostLen, const char *request_method, int keep_alive, unsigned int ip, unsigned int port);
void http_query_data_free(http_query_data *d);

/** rpc_query_data **/
struct rpc_query_data {
  int *data, len;

  long long req_id;

  /** PID **/
  unsigned ip;
  short port;
  short pid;
  int utime;
};

rpc_query_data *rpc_query_data_create(int *data, int len, long long req_id, unsigned int ip, short port, short pid, int utime);
void rpc_query_data_free(rpc_query_data *d);

/** php_query_data **/
struct php_query_data {
  http_query_data *http_data;
  rpc_query_data *rpc_data;
};

php_query_data *php_query_data_create(http_query_data *http_data, rpc_query_data *rpc_data);
void php_query_data_free(php_query_data *d);


/** rpc intreface **/
using slot_id_t = int;

enum net_event_type_t {
  ne_rpc_answer,
  ne_rpc_error
};

struct net_event_t {
  net_event_type_t type;
  union {
    slot_id_t slot_id;
    slot_id_t rpc_id;
  };
  union {
    struct { //ne_rpc_answer
      int result_len;
      //allocated via dl_malloc
      char *result;
    };
    struct { //ne_rpc_error
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
