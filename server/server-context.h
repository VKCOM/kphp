// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <sstream>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

#include "common/algorithms/string-algorithms.h"
#include "common/dl-utils-lite.h"
#include "common/kprintf.h"
#include "net/net-socket.h"

template<std::size_t MAX_PORTS_ = std::numeric_limits<std::size_t>::max()>
class ServerContext {
public:
  static constexpr size_t MAX_PORTS = MAX_PORTS_;

  bool init_from_option(const char* option);
  bool server_enabled() const noexcept;
  const std::vector<uint16_t>& ports() const noexcept;
  const std::vector<int>& socket_fds() const noexcept;

  bool master_create_server_sockets();
  void master_set_open_sockets(std::vector<int>&& socket_fds);

  void dedicate_server_socket_to_worker(uint16_t worker_unique_id);
  int worker_socket_fd() const noexcept;
  int worker_port() const noexcept;

private:
  std::vector<uint16_t> ports_;
  std::vector<int> socket_fds_;

  int cur_worker_socket_idx_{-1};
};

template<size_t MAX_PORTS>
const std::vector<uint16_t>& ServerContext<MAX_PORTS>::ports() const noexcept {
  return ports_;
}

template<size_t MAX_PORTS>
const std::vector<int>& ServerContext<MAX_PORTS>::socket_fds() const noexcept {
  return socket_fds_;
}

template<size_t MAX_PORTS>
bool ServerContext<MAX_PORTS>::server_enabled() const noexcept {
  return !ports_.empty();
}

template<size_t MAX_PORTS>
bool ServerContext<MAX_PORTS>::init_from_option(const char* option) {
  std::stringstream ss(option);
  std::string segment;

  while(std::getline(ss, segment, ',')) {
    auto trimmed = vk::trim(segment);
    int val = std::atoi(trimmed.data());
    auto port = static_cast<uint16_t>(val);
    if (port != val) {
      kprintf("Incorrect port %d\n", val);
      return false;
    }
    ports_.emplace_back(port);
  }

  // sort and remove duplicates:
  std::sort(ports_.begin(), ports_.end());
  ports_.erase(std::unique(ports_.begin(), ports_.end() ), ports_.end());

  if (ports_.size() > MAX_PORTS) {
    kprintf("Can't listen more than %" PRIu64 " ports\n", MAX_PORTS);
    return false;
  }

  return true;
}

template<size_t MAX_PORTS>
bool ServerContext<MAX_PORTS>::master_create_server_sockets() {
  socket_fds_.reserve(ports_.size());
  for (auto port : ports_) {
    int socket = server_socket(port, settings_addr, backlog, 0);
    if (socket == -1) {
      return false;
    }
    socket_fds_.emplace_back(socket);
  }
  return true;
}

template<size_t MAX_PORTS>
void ServerContext<MAX_PORTS>::master_set_open_sockets(std::vector<int>&& socket_fds) {
  dl_assert(socket_fds.size() == ports_.size(),
            dl_pstr("Got inconsistent number of open http sockets. Expected %zu, but got %zu", ports_.size(), socket_fds.size()));
  socket_fds_ = std::move(socket_fds);
}

template<size_t MAX_PORTS>
void ServerContext<MAX_PORTS>::dedicate_server_socket_to_worker(uint16_t worker_unique_id) {
  if (!server_enabled()) {
    return;
  }
  cur_worker_socket_idx_ = worker_unique_id % socket_fds_.size();
  for (int i = 0; i < socket_fds_.size(); ++i) {
    if (i != cur_worker_socket_idx_) {
      close(socket_fds_[i]);
    }
  }
}

template<size_t MAX_PORTS>
int ServerContext<MAX_PORTS>::worker_socket_fd() const noexcept {
  if (!server_enabled()) {
    return -1;
  }
  return socket_fds_[cur_worker_socket_idx_];
}

template<size_t MAX_PORTS>
int ServerContext<MAX_PORTS>::worker_port() const noexcept {
  if (!server_enabled()) {
    return -1;
  }
  return ports_[cur_worker_socket_idx_];
}
