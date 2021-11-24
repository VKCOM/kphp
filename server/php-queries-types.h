// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

enum class protocol_type {
  memcached,
  mysqli,
  rpc,
};

struct php_worker;

struct php_query_base_t {
  void *ans{nullptr};

  virtual void run(php_worker *worker) noexcept = 0;

  virtual ~php_query_base_t() = default;
};

struct php_query_x2_t : php_query_base_t {
  int val{0};

  void run(php_worker *worker) noexcept final;
};

struct php_query_rpc_answer : php_query_base_t {
  const char *data{nullptr};
  int data_len{0};

  void run(php_worker *worker) noexcept final;
};

struct php_query_connect_t : php_query_base_t {
  const char *host{nullptr};
  int port{0};
  protocol_type protocol{};

  void run(php_worker *worker) noexcept final;
};

class Connector;

struct external_driver_connect : php_query_base_t {
  Connector *connector;

  explicit external_driver_connect(Connector *connector) : connector(connector) {};

  void run(php_worker *worker) noexcept final;
};

struct php_query_http_load_post_t : php_query_base_t {
  char *buf{nullptr};
  int min_len{0};
  int max_len{0};

  void run(php_worker *worker) noexcept final;
};

struct php_net_query_packet_t : php_query_base_t {
  int connection_id{0};
  const char *data{nullptr};
  int data_len{0};
  long long data_id{0};

  double timeout{0.0};
  protocol_type protocol{};
  int extra_type{0};

  void run(php_worker *worker) noexcept final;
};

struct php_query_wait_t : php_query_base_t {
  int timeout_ms{0};

  void run(php_worker *worker) noexcept final;
};
