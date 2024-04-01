// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/http-server-context.h"

#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "common/algorithms/string-algorithms.h"
#include "common/dl-utils-lite.h"
#include "common/kprintf.h"
#include "net/net-socket.h"

const std::vector<uint16_t> &HttpServerContext::http_ports() const noexcept {
  return http_ports_;
}

const std::vector<int> &HttpServerContext::http_socket_fds() const noexcept {
  return http_sfds_;
}

bool HttpServerContext::http_server_enabled() const noexcept {
  return !http_ports_.empty();
}

bool HttpServerContext::init_from_option(const char *option) {
  std::stringstream ss(option);
  std::string segment;

  while (std::getline(ss, segment, ',')) {
    auto trimmed = vk::trim(segment);
    int val = std::atoi(trimmed.data());
    auto port = static_cast<uint16_t>(val);
    if (port != val) {
      kprintf("Incorrect port %d\n", val);
      return false;
    }
    http_ports_.emplace_back(port);
  }

  // sort and remove duplicates:
  std::sort(http_ports_.begin(), http_ports_.end());
  http_ports_.erase(std::unique(http_ports_.begin(), http_ports_.end()), http_ports_.end());

  if (http_ports_.size() > MAX_HTTP_PORTS) {
    kprintf("Can't listen more than %d HTTP ports\n", MAX_HTTP_PORTS);
    return false;
  }

  return true;
}

bool HttpServerContext::master_create_http_sockets() {
  http_sfds_.reserve(http_ports_.size());
  for (auto port : http_ports_) {
    int socket = server_socket(port, settings_addr, backlog, 0);
    if (socket == -1) {
      return false;
    }
    http_sfds_.emplace_back(socket);
  }
  return true;
}

void HttpServerContext::master_set_open_http_sockets(std::vector<int> &&open_http_sockets) {
  dl_assert(open_http_sockets.size() == http_ports_.size(),
            dl_pstr("Got inconsistent number of open http sockets. Expected %zu, but got %zu", http_ports_.size(), open_http_sockets.size()));
  http_sfds_ = std::move(open_http_sockets);
}

void HttpServerContext::dedicate_http_socket_to_worker(uint16_t worker_unique_id) {
  if (!http_server_enabled()) {
    return;
  }
  cur_worker_socket_idx_ = worker_unique_id % http_sfds_.size();
  for (int i = 0; i < http_sfds_.size(); ++i) {
    if (i != cur_worker_socket_idx_) {
      close(http_sfds_[i]);
    }
  }
}

int HttpServerContext::worker_http_socket_fd() const noexcept {
  if (!http_server_enabled()) {
    return -1;
  }
  return http_sfds_[cur_worker_socket_idx_];
}

int HttpServerContext::worker_http_port() const noexcept {
  if (!http_server_enabled()) {
    return -1;
  }
  return http_ports_[cur_worker_socket_idx_];
}
