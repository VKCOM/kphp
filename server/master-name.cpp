// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/master-name.h"

#include <algorithm>

#include <cctype>
#include <cinttypes>
#include <cstdio>
#include <cstring>

#include "common/crc32.h"

MasterName::MasterName() {
  set_master_name(DEFAULT_MASTER_NAME, false);
}

const char *MasterName::set_master_name(const char *name, bool deprecated) noexcept {
  static const char *socket_suffix = "_kphp_fd_transfer";
  static const char *shm_prefix = "/";
  static const char *shm_suffix = "_kphp_shm";
  static const char *statsd_prefix = "kphp_stats.";
  static const auto reserved = std::max({std::strlen(socket_suffix), std::strlen(shm_prefix) + std::strlen(shm_suffix), std::strlen(statsd_prefix)});


  const auto name_len = std::strlen(name);
  if (!name_len) {
    return "empty master name";
  }
  if (name_len + reserved >= master_name_.size()) {
    return "too long master name";
  }
  if (deprecated && strcmp(master_name_.data(), DEFAULT_MASTER_NAME)) {
    return nullptr;
  }

  std::copy(name, name + name_len + 1, master_name_.begin());
  bool has_wrong_symbols = std::any_of(master_name_.begin(), master_name_.begin() + name_len, [](char c) {
    return !std::isalnum(c) && c != '-' && c != '_';
  });
  if (has_wrong_symbols) {
    return "Incorrect symbol in master name. Allowed symbols are: alpha-numerics, '-', '_'";
  }

  if (std::strlen(socket_suffix) + name_len > MAX_SOCKET_NAME_LEN) {
    // To allow master name longer than 107 symbols, we just take crc64 of it as the socket name
    uint64_t crc = compute_crc64(name, name_len);
    size_t hex_crc_str_size = sizeof(crc) * 2 + 1;
    char hex_crc_str[hex_crc_str_size];
    std::snprintf(hex_crc_str, hex_crc_str_size, "%16" PRIx64, crc);
    std::strcpy(socket_name_.data(), hex_crc_str);
  } else {
    std::strcpy(socket_name_.data(), master_name_.data());
  }
  std::strcat(socket_name_.data(), socket_suffix);

  std::strcpy(shmem_name_.data(), shm_prefix);
  std::strcat(shmem_name_.data(), master_name_.data());
  std::strcat(shmem_name_.data(), shm_suffix);

  return nullptr;
}
