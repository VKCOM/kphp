#pragma once

#include "runtime-light/core/kphp_core.h"
#include "runtime-light/coroutine/task.h"

struct Superglobals {
  mixed v$_SERVER;
  mixed v$_GET;
  mixed v$_POST;
  mixed v$_FILES;
  mixed v$_COOKIE;
  mixed v$_REQUEST;
  mixed v$_ENV;

  mixed v$argc;
  mixed v$argv;
};

struct http_query_data {
  char const *uri, *get, *headers, *post, *request_method;
  int uri_len, get_len, headers_len, post_len, request_method_len;
  int keep_alive;
  unsigned int ip;
  unsigned int port;
};

void init_superglobals(const char * buffer, int size);
