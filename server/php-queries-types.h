// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

struct php_worker;

struct php_query_base_t {
  void *ans;

  virtual void run(php_worker *worker) noexcept = 0;

  virtual ~php_query_base_t() = default;
};

struct php_query_x2_t : php_query_base_t {
  int val;

  void run(php_worker *worker) noexcept final;
};

struct php_query_rpc_answer : php_query_base_t {
  const char *data;
  int data_len;

  void run(php_worker *worker) noexcept final;
};

enum class protocol_type {
  memcached,
  mysqli,
  rpc,
};

struct php_query_connect_t : php_query_base_t {
  const char *host;
  int port;
  protocol_type protocol;

  void run(php_worker *worker) noexcept final;
};

struct php_query_http_load_post_t : php_query_base_t {
  char *buf;
  int min_len;
  int max_len;

  void run(php_worker *worker) noexcept final;
};

struct php_net_query_packet_t : php_query_base_t {
  int connection_id;
  const char *data;
  int data_len;
  long long data_id;

  double timeout;
  protocol_type protocol;
  int extra_type;

  void run(php_worker *worker) noexcept final;
};

struct php_query_wait_t : php_query_base_t {
  int timeout_ms;

  void run(php_worker *worker) noexcept final;
};
