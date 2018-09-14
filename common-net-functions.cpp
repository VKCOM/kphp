#include "PHP/common-net-functions.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "drinkless/dl-utils-lite.h"

http_query_data *http_query_data_create(
            const char *qUri, int qUriLen,
            const char *qGet, int qGetLen,
            const char *qHeaders, int qHeadersLen,
            const char *qPost, int qPostLen,
            const char *request_method, int keep_alive, unsigned int ip, unsigned int port) {
  http_query_data *d = (http_query_data *)dl_malloc(sizeof(http_query_data));

  //TODO remove memdup completely. We can just copy pointers
  d->uri = (char *)dl_memdup(qUri, qUriLen);
  d->get = (char *)dl_memdup(qGet, qGetLen);
  d->headers = (char *)dl_memdup(qHeaders, qHeadersLen);
  if (qPost != nullptr) {
    d->post = (char *)dl_memdup(qPost, qPostLen);
  } else {
    d->post = nullptr;
  }

  d->uri_len = qUriLen;
  d->get_len = qGetLen;
  d->headers_len = qHeadersLen;
  d->post_len = qPostLen;

  d->request_method = (char *)dl_memdup(request_method, strlen(request_method));
  d->request_method_len = (int)strlen(request_method);

  d->keep_alive = keep_alive;

  d->ip = ip;
  d->port = port;

  return d;
}

void http_query_data_free(http_query_data *d) {
  if (d == nullptr) {
    return;
  }

  dl_free(d->uri, d->uri_len);
  dl_free(d->get, d->get_len);
  dl_free(d->headers, d->headers_len);
  dl_free(d->post, d->post_len);

  dl_free(d->request_method, d->request_method_len);

  dl_free(d, sizeof(http_query_data));
}

rpc_query_data *rpc_query_data_create(int *data, int len, long long req_id, unsigned int ip, short port, short pid, int utime) {
  rpc_query_data *d = (rpc_query_data *)dl_malloc(sizeof(rpc_query_data));

  d->data = (int *)dl_memdup(data, sizeof(int) * len);
  d->len = len;

  d->req_id = req_id;

  d->ip = ip;
  d->port = port;
  d->pid = pid;
  d->utime = utime;

  return d;
}

void rpc_query_data_free(rpc_query_data *d) {
  if (d == nullptr) {
    return;
  }

  dl_free(d->data, d->len * sizeof(int));
  dl_free(d, sizeof(rpc_query_data));
}

php_query_data *php_query_data_create(http_query_data *http_data, rpc_query_data *rpc_data) {
  php_query_data *d = (php_query_data *)dl_malloc(sizeof(php_query_data));

  d->http_data = http_data;
  d->rpc_data = rpc_data;

  return d;
}

void php_query_data_free(php_query_data *d) {
  http_query_data_free(d->http_data);
  rpc_query_data_free(d->rpc_data);

  dl_free(d, sizeof(php_query_data));
}


#define critical_error(s)             \
  fprintf (stderr, "%s called\n", s); \
  assert (0);


static int mc_connect_to_default(const char *host_name __attribute__((unused)), int port __attribute__((unused))) {
  critical_error (__FUNCTION__);
}

static void mc_run_query_default(int host_num __attribute__((unused)), const char *request __attribute__((unused)), int request_len __attribute__((unused)), int timeout_ms __attribute__((unused)), int query_type __attribute__((unused)), void (*callback)(const char *result, int result_len) __attribute__((unused))) {
  critical_error (__FUNCTION__);
}


static int db_proxy_connect_default(void) {
  critical_error (__FUNCTION__);
}

static void db_run_query_default(int host_num __attribute__((unused)), const char *request __attribute__((unused)), int request_len __attribute__((unused)), int timeout_ms __attribute__((unused)), void (*callback)(const char *result, int result_len) __attribute__((unused))) {
  critical_error (__FUNCTION__);
}


static void http_set_result_default(const char *headers __attribute__((unused)), int headers_len __attribute__((unused)), const char *body __attribute__((unused)), int body_len __attribute__((unused)), int exit_code __attribute__((unused))) {
  critical_error (__FUNCTION__);
}

static int http_load_long_query_default(char *buf __attribute__((unused)), int min_len __attribute__((unused)), int max_len __attribute__((unused))) {
  critical_error (__FUNCTION__);
}


static void set_server_status_default(const char *status __attribute__((unused)), int status_len __attribute__((unused))) {
  critical_error (__FUNCTION__);
}

static void set_server_status_rpc_default(int port __attribute__((unused)), long long actor_id __attribute__((unused)), double start_time __attribute__((unused))) {
  critical_error (__FUNCTION__);
}

static double get_net_time_default(void) {
  critical_error (__FUNCTION__);
}

double get_script_time_default(void) {
  critical_error (__FUNCTION__);
}

int get_net_queries_count_default(void) {
  critical_error (__FUNCTION__);
}


int get_engine_uptime_default(void) {
  critical_error (__FUNCTION__);
}

const char *get_engine_version_default(void) {
  critical_error (__FUNCTION__);
}


static void rpc_answer_default(const char *res __attribute__((unused)), int res_len __attribute__((unused))) {
  critical_error (__FUNCTION__);
}

static void rpc_set_result_default(const char *body __attribute__((unused)), int body_len __attribute__((unused)), int exit_code __attribute__((unused))) {
  critical_error (__FUNCTION__);
}

static void script_error_default(void) {
  critical_error (__FUNCTION__);
}


static void finish_script_default(int exit_code) {
  exit(exit_code);
}


static int rpc_connect_to_default(const char *host_name __attribute__((unused)), int port __attribute__((unused))) {
  critical_error (__FUNCTION__);
}

static slot_id_t rpc_send_query_default(int host_num __attribute__((unused)), char *request __attribute__((unused)), int request_len __attribute__((unused)), int timeout_ms __attribute__((unused))) {
  critical_error (__FUNCTION__);
}

static void wait_net_events_default(int timeout_ms __attribute__((unused))) {
  return;
}

static net_event_t *pop_net_event_default(void) {
  return nullptr;
}


static int query_x2_default(int x __attribute__((unused))) {
  critical_error (__FUNCTION__);
}


/***
  Function pointers
 ***/


int (*mc_connect_to)(const char *host_name, int port) = mc_connect_to_default;

void (*mc_run_query)(int host_num, const char *request, int request_len, int timeout_ms, int query_type, void (*callback)(const char *result, int result_len)) = mc_run_query_default;


int (*db_proxy_connect)(void) = db_proxy_connect_default;

void (*db_run_query)(int host_num, const char *request, int request_len, int timeout_ms, void (*callback)(const char *result, int result_len)) = db_run_query_default;


void (*http_set_result)(const char *headers, int headers_len, const char *body, int body_len, int exit_code) = http_set_result_default;

int (*http_load_long_query)(char *buf, int min_len, int max_len) = http_load_long_query_default;


void (*set_server_status)(const char *status, int status_len) = set_server_status_default;
void (*set_server_status_rpc)(int port, long long actor_id, double start_time) = set_server_status_rpc_default;


double (*get_net_time)(void) = get_net_time_default;

double (*get_script_time)(void) = get_script_time_default;

int (*get_net_queries_count)(void) = get_net_queries_count_default;


int (*get_engine_uptime)(void) = get_engine_uptime_default;

const char *(*get_engine_version)(void) = get_engine_version_default;


void (*rpc_answer)(const char *res, int res_len) = rpc_answer_default;

void (*rpc_set_result)(const char *body, int body_len, int exit_code) = rpc_set_result_default;

void (*script_error)(void) = script_error_default;


void (*finish_script)(int exit_code) = finish_script_default;


int (*rpc_connect_to)(const char *host_name, int port) = rpc_connect_to_default;

slot_id_t (*rpc_send_query)(int host_num, char *request, int request_len, int timeout_ms) = rpc_send_query_default;

void (*wait_net_events)(int timeout_ms) = wait_net_events_default;

net_event_t *(*pop_net_event)(void) = pop_net_event_default;


int (*query_x2)(int x) = query_x2_default;
