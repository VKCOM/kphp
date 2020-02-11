#include "server/lease-config-parser.h"

#include <algorithm>

#include "auto/TL/constants/kphp.h"
#include "common/kprintf.h"

#include "server/php-engine-vars.h"

vk::optional<QueueTypesLeaseWorkerMode> LeaseConfigParser::get_lease_mode_from_config(const YAML::Node &node) {
  vk::optional<QueueTypesLeaseWorkerMode> lease_worker_mode;
  if (const auto &type_names = node["type_names"]) {
    QueueTypesLeaseWorkerMode mode;
    if (type_names.size() == 0) {
      throw std::runtime_error("List of queue types 'type_names' is empty. Lease worker will be idle.");
    }
    if (const auto &queue_id = node["queue_id"]) {
      if (queue_id.size() == 0) {
        throw std::runtime_error("'queue_id' list is empty. Lease worker will be idle.");
      }
      mode.fields_mask |= vk::tl::kphp::queue_types_mode_fields_mask::queue_id;
      mode.queue_id.resize(queue_id.size());
      std::transform(queue_id.begin(), queue_id.end(), mode.queue_id.begin(), [](const YAML::Node &node) { return node.as<int>(); });
    }
    mode.type_names.resize(type_names.size());
    std::transform(type_names.begin(), type_names.end(), mode.type_names.begin(), [](const YAML::Node &node) { return node.as<std::string>(); });
    lease_worker_mode.emplace(mode);
  } else if (const auto &_ = node["queue_id"]) {
    throw std::runtime_error("List of queue types 'type_names' is not specified.");
  }
  return lease_worker_mode;
}

bool LeaseConfigParser::parse_lease_options_config(const char *lease_config) {
  try {
    YAML::Node node = YAML::LoadFile(lease_config);
    cur_lease_mode = LeaseConfigParser::get_lease_mode_from_config(node);
  } catch (const std::exception &e) {
    kprintf("-S option, incorrect lease config '%s'\n%s\n", lease_config, e.what());
    return false;
  }
  return true;
}
