#pragma once

#include <yaml-cpp/yaml.h>

#include "server/lease-worker-mode.h"

class LeaseConfigParser {
public:
  static bool parse_lease_options_config(const char *lease_config);
private:
  static vk::optional<QueueTypesLeaseWorkerMode> get_lease_mode_from_config(const YAML::Node &node);
};
