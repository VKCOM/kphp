#pragma once

#include <yaml-cpp/yaml.h>

#include "common/kphp-tasks-lease/lease-worker-mode.h"

#include "server/lease-rpc-client.h"

/* Пример конфига:
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
private:
  static vk::optional<QueueTypesLeaseWorkerMode> get_lease_mode_from_config(const YAML::Node &node);
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
