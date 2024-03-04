// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <climits>
#include <yaml-cpp/yaml.h>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"

class ServerConfig : vk::not_copyable {
public:
  const char *get_cluster_name() const noexcept {
    return cluster_name_.data();
  }

  const char *get_statsd_prefix() const noexcept {
    return statsd_prefix_.data();
  }

  const char *set_cluster_name(const char *cluster_name, bool deprecated) noexcept;

  int init_from_config(const char *config_path) noexcept;

private:
  const char *DEFAULT_CLUSTER_NAME = "default";

  ServerConfig();

  friend class vk::singleton<ServerConfig>;

  std::array<char, NAME_MAX> cluster_name_;
  std::array<char, NAME_MAX> statsd_prefix_;
};
