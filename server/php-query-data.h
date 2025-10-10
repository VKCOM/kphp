// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <variant>
#include <cstring>

#include "common/tl/query-header.h"
#include "common/dl-utils-lite.h"


struct http_query_data {
  char const *uri, *get, *headers, *post, *request_method;
  int uri_len, get_len, headers_len, post_len, request_method_len;
  int keep_alive;
  unsigned int ip;
  unsigned int port;
};

struct rpc_query_data {
  tl_query_header_t header;
  std::vector<char> data;
  process_id remote_pid;
};

namespace job_workers {
  struct JobSharedMessage;
} // namespace job_workers

struct job_query_data {
  job_workers::JobSharedMessage *job_request;
  const char *(*send_reply) (job_workers::JobSharedMessage *job_response);
};

using null_query_data = std::monostate;

using php_query_data_t = std::variant<null_query_data,
                                      http_query_data,
                                      rpc_query_data,
                                      job_query_data>;
