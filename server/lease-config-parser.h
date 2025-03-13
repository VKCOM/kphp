// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "yaml-cpp/yaml.h"
#include <optional>

#include "common/kphp-tasks-lease/lease-worker-mode.h"

#include "server/lease-rpc-client.h"

/* Config example:
------------- tasks-config.yaml -------------
rpc_clients:
- { host: "vk.com", port: 1234, actor: 1 }
- { host: "another.vk.com", port: 2222 }

type_names:
- type1
- type2
- type3

queue_id:
- 1
- 2

*/

class LeaseConfigParser {
public:
  static int parse_lease_options_config(const char *lease_config) noexcept;
  static char *lease_config_path;
private:
  static std::optional<QueueTypesLeaseWorkerMode> get_lease_mode_from_config(const YAML::Node &node);
  static std::optional<QueueTypesLeaseWorkerModeV2> get_lease_mode_v2_from_config(const YAML::Node &node);
  static std::vector<LeaseRpcClient> get_rpc_clients_from_config(const YAML::Node &node);

  template <typename T>
  static T get_typed_field(const YAML::Node &node, const std::string &field_name, bool silent = false, const T &default_val = {}) {
    if (const auto &res = node[field_name]) {
      return res.as<T>();
    }
    if (!silent) {
      throw std::runtime_error(field_name + " is not specified in some rpc_client");
    }
    return default_val;
  }
};
