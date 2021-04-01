// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <algorithm>
#include <cstring>
#include <cctype>

#include "server/cluster-name.h"

ClusterName::ClusterName() {
  set_cluster_name("default");
}

const char *ClusterName::set_cluster_name(const char *name) noexcept {
  // reserved for shmem, statsd, socket suffixes and prefixes
  constexpr size_t reserved = 32;
  const auto name_len = std::strlen(name);
  if (!name_len) {
    return "empty cluster name";
  }
  if (name_len + reserved >= cluster_name_.size()) {
    return "too long cluster name";
  }
  std::copy(name, name + name_len + 1, cluster_name_.begin());
  std::replace_copy_if(name, name + name_len + 1, socket_name_.begin(),
                       [](char c) { return c && !std::isalpha(c); }, '_');

  std::strcpy(shmem_name_.data(), "/");
  std::strcat(shmem_name_.data(), socket_name_.data());
  std::strcat(shmem_name_.data(), "_kphp_shm");

  std::strcpy(statsd_prefix_.data(), "kphp_stats.");
  std::strcat(statsd_prefix_.data(), socket_name_.data());

  std::strcat(socket_name_.data(), "_kphp_fd_transfer");
  return nullptr;
}
