// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <vector>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"

class HttpServerContext : vk::not_copyable {
public:
  static constexpr int MAX_HTTP_PORTS = 64;

  bool init_from_option(const char *option);
  bool http_server_enabled() const noexcept;
  const std::vector<uint16_t> &http_ports() const noexcept;
  const std::vector<int> &http_socket_fds() const noexcept;

  bool master_create_http_sockets();
  void master_set_open_http_sockets(std::vector<int> &&http_sfds);

  void dedicate_http_socket_to_worker(uint16_t worker_unique_id);
  int worker_http_socket_fd() const noexcept;
  int worker_http_port() const noexcept;

private:
  std::vector<uint16_t> http_ports_;
  std::vector<int> http_sfds_;

  int cur_worker_socket_idx_{-1};

  HttpServerContext() = default;

  friend class vk::singleton<HttpServerContext>;
};
