// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <climits>
#include <sys/un.h>
#include <type_traits>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"

class ClusterName : vk::not_copyable {
public:
  const char *set_cluster_name(const char *cluster_name) noexcept;

  const char *get_cluster_name() const noexcept { return cluster_name_.data(); }
  const char *get_socket_name() const noexcept { return socket_name_.data(); }
  const char *get_shmem_name() const noexcept { return shmem_name_.data(); }
  const char *get_statsd_prefix() const noexcept { return statsd_prefix_.data(); }

private:
  static constexpr size_t MAX_SOCKET_NAME_LEN = sizeof(std::declval<sockaddr_un>().sun_path) - 1;

  ClusterName();

  friend class vk::singleton<ClusterName>;

  std::array<char, NAME_MAX> cluster_name_;
  std::array<char, MAX_SOCKET_NAME_LEN + 1> socket_name_;
  std::array<char, NAME_MAX> shmem_name_;
  std::array<char, NAME_MAX> statsd_prefix_;
};
