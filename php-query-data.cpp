#include "PHP/php-query-data.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

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
