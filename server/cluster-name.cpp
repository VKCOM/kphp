// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/cluster-name.h"

#include <algorithm>

#include <cctype>
#include <cinttypes>
#include <cstdio>
#include <cstring>

#include "common/crc32.h"

ClusterName::ClusterName() {
  set_cluster_name("default");
}

const char *ClusterName::set_cluster_name(const char *name) noexcept {
  // reserved for shmem, statsd, socket suffixes and prefixes
  constexpr size_t reserved = 42;
  const auto name_len = std::strlen(name);
  if (!name_len) {
    return "empty cluster name";
  }
  if (name_len + reserved >= cluster_name_.size()) {
    return "too long cluster name";
  }
  std::copy(name, name + name_len + 1, cluster_name_.begin());
  bool has_wrong_symbols = std::any_of(cluster_name_.begin(), cluster_name_.begin() + name_len, [](char c) {
    return !std::isalnum(c) && c != '-' && c != '_';
  });
  if (has_wrong_symbols) {
    return "Incorrect symbol in cluster name. Allowed symbols are: alpha-numerics, '-', '_'";
  }

  const char *socket_prefix = "kphp_master_fd_for_graceful_restart-";
  auto socket_prefix_len = std::strlen(socket_prefix);
  std::strcpy(socket_name_.data(), socket_prefix);
  if (socket_prefix_len + name_len > MAX_SOCKET_NAME_LEN) {
    // To allow cluster name longer than 107 symbols, we just take crc64 of it as the socket name
    uint64_t crc = compute_crc64(name, name_len);
    char hex_crc_str[sizeof(crc) * 2 + 1];
    std::sprintf(hex_crc_str, "%16" PRIx64, crc);
    std::strcat(socket_name_.data(), hex_crc_str);
  } else {
    std::strcat(socket_name_.data(), cluster_name_.data());
  }

  std::strcpy(shmem_name_.data(), "/");
  std::strcat(shmem_name_.data(), cluster_name_.data());
  std::strcat(shmem_name_.data(), "_kphp_shm");

  std::strcpy(statsd_prefix_.data(), "kphp_stats.");
  std::strcat(statsd_prefix_.data(), cluster_name_.data());
  return nullptr;
}
