// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>

enum class protocol_type {
  memcached,
  mysqli,
  rpc,
};

class PhpWorker;

struct php_query_base_t {
  void *ans{nullptr};

  virtual void run(PhpWorker *worker) noexcept = 0;

  virtual ~php_query_base_t() = default;
};

struct php_query_x2_t : php_query_base_t {
  int val{0};

  void run(PhpWorker *worker) noexcept final;
};

struct php_query_rpc_answer : php_query_base_t {
  const char *data{nullptr};
  int data_len{0};

  void run(PhpWorker *worker) noexcept final;
};

struct php_query_connect_t : php_query_base_t {
  const char *host{nullptr};
  int port{0};
  protocol_type protocol{};

  void run(PhpWorker *worker) noexcept final;
};

namespace database_drivers {
class Connector;
} // namespace database_drivers

struct external_driver_connect : php_query_base_t {
  std::unique_ptr<database_drivers::Connector> connector;

  explicit external_driver_connect(std::unique_ptr<database_drivers::Connector> &&connector);

  void run(PhpWorker *worker) noexcept final;
};

struct php_query_http_load_post_t : php_query_base_t {
  char *buf{nullptr};
  int min_len{0};
  int max_len{0};

  void run(PhpWorker *worker) noexcept final;
};

struct php_net_query_packet_t : php_query_base_t {
  int connection_id{0};
  const char *data{nullptr};
  int data_len{0};
  long long data_id{0};

  double timeout{0.0};
  protocol_type protocol{};
  int extra_type{0};

  void run(PhpWorker *worker) noexcept final;
};

struct php_query_wait_t : php_query_base_t {
  int timeout_ms{0};

  void run(PhpWorker *worker) noexcept final;
};
