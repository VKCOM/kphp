// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/php-query-data.h"

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "common/dl-utils-lite.h"

static void *memdup(const void *src, size_t x) {
  if (!src) {
    assert(!x);
    return nullptr;
  }
  void *res = malloc(x);
  assert(res);
  memcpy(res, src, x);
  return res;
}

http_query_data *http_query_data_create(
            const char *qUri, int qUriLen,
            const char *qGet, int qGetLen,
            const char *qHeaders, int qHeadersLen,
            const char *qPost, int qPostLen,
            const char *request_method, int keep_alive, unsigned int ip, unsigned int port) {
  http_query_data *d = (http_query_data *)malloc(sizeof(http_query_data));

  //TODO remove memdup completely. We can just copy pointers
  d->uri = (char *)memdup(qUri, qUriLen);
  d->get = (char *)memdup(qGet, qGetLen);
  d->headers = (char *)memdup(qHeaders, qHeadersLen);
  if (qPost) {
    d->post = (char *)memdup(qPost, qPostLen);
  } else {
    d->post = nullptr;
  }
  d->uri_len = qUriLen;
  d->get_len = qGetLen;
  d->headers_len = qHeadersLen;
  d->post_len = qPostLen;

  d->request_method = (char *)memdup(request_method, strlen(request_method));
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

  free(d->uri);
  free(d->get);
  free(d->headers);
  free(d->post);

  free(d->request_method);

  free(d);
}

rpc_query_data *rpc_query_data_create(tl_query_header_t &&header, int *data, int len, unsigned int ip, short port, short pid, int utime) {
  auto *d = new rpc_query_data;

  d->header = std::move(header);

  d->data = static_cast<int *>(memdup(data, sizeof(int) * len));
  d->len = len;

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

  free(d->data);
  delete d;
}

php_query_data *php_query_data_create(http_query_data *http_data, rpc_query_data *rpc_data, job_query_data *job_data) {
  php_query_data *d = (php_query_data *)malloc(sizeof(php_query_data));

  d->http_data = http_data;
  d->rpc_data = rpc_data;
  d->job_data = job_data;

  return d;
}

void php_query_data_free(php_query_data *d) {
  http_query_data_free(d->http_data);
  rpc_query_data_free(d->rpc_data);
  job_query_data_free(d->job_data);

  free(d);
}
