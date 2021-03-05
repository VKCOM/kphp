// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/tl/query-header.h"
#include "server/job-workers/job.h"

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
  tl_query_header_t header;

  int *data, len;

  /** PID **/
  unsigned ip;
  short port;
  short pid;
  int utime;
};

rpc_query_data *rpc_query_data_create(tl_query_header_t &&header, int *data, int len, unsigned int ip, short port, short pid, int utime);
void rpc_query_data_free(rpc_query_data *d);

struct job_query_data {
  job_workers::Job job;
  const char *(*send_reply) (job_workers::SharedMemorySlice *reply_memory);
};

inline job_query_data *job_query_data_create(const job_workers::Job &job, const char *(*send_reply) (job_workers::SharedMemorySlice *reply_memory)) {
  return new job_query_data{job, send_reply};
}

inline void job_query_data_free(job_query_data *job_data) {
  if (job_data == nullptr) {
    return;
  }
  delete job_data;
}

/** php_query_data **/
struct php_query_data {
  http_query_data *http_data;
  rpc_query_data *rpc_data;
  job_query_data *job_data;
};

php_query_data *php_query_data_create(http_query_data *http_data, rpc_query_data *rpc_data, job_query_data *job_data);
void php_query_data_free(php_query_data *d);


