// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/server-config.h"
#include "common/kprintf.h"
#include "common/resolver.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <exception>
#include <iostream>
#include <yaml-cpp/yaml.h>

ServerConfig::ServerConfig() {
  set_cluster_name(DEFAULT_CLUSTER_NAME, false);
}

bool check_wrong_symbols(char c) {
  return !std::isalnum(c) && c != '-' && c != '_';
}

/**
 * temporarily for backward compatibility
 */
const char *ServerConfig::set_cluster_name(const char *cluster_name, bool deprecated) noexcept {
  if (deprecated && strcmp(cluster_name_.data(), DEFAULT_CLUSTER_NAME)) {
    return nullptr;
  }
  static const char *statsd_prefix = "kphp_stats.";
  static const size_t reserved = std::strlen(statsd_prefix);

  const auto name_len = std::strlen(cluster_name);
  if (!name_len) {
    return "empty cluster name";
  }
  if (name_len + reserved >= cluster_name_.size()) {
    return "too long cluster name";
  }
  std::copy(cluster_name, cluster_name + name_len + 1, cluster_name_.begin());
  bool has_wrong_symbols = std::any_of(cluster_name_.begin(), cluster_name_.begin() + name_len, check_wrong_symbols);
  if (has_wrong_symbols) {
    return "Incorrect symbol in cluster name. Allowed symbols are: alpha-numerics, '-', '_'";
  }

  std::strcpy(statsd_prefix_.data(), statsd_prefix);
  std::strcat(statsd_prefix_.data(), cluster_name_.data());

  return nullptr;
};

int ServerConfig::init_from_config(const char *config_path) noexcept {
  if (!config_path) {
    kprintf("--server-config option empty server config path\n");
    return -1;
  }

  try {
    YAML::Node node = YAML::LoadFile(config_path);
    const std::string cluster_name = node["cluster_name"].as<std::string>();
    if (const auto *err_msg = set_cluster_name(cluster_name.data(), false)) {
      throw std::runtime_error(err_msg);
    }
    auto server_name_node = node["server_name"];
    std::string server_name;
    if (server_name_node.IsNull() || !server_name_node.IsDefined()) {
      const auto *hostname = kdb_gethostname();
      server_name.assign(hostname, std::strlen(hostname));
    } else {
      server_name = server_name_node.as<std::string>();
      if (std::any_of(server_name.begin(), server_name.end(), check_wrong_symbols)) {
        kprintf("Incorrect symbol in server name. Allowed symbols are: alpha-numerics, '-', '_'");
        return -1;
      }
    }
    std::copy(server_name.begin(), server_name.end(), server_name_.begin());
  } catch (const std::exception &e) {
    kprintf("--server-config, incorrect server config: '%s'\n%s\n", config_path, e.what());
    return -1;
  }
  return 0;
}
