#pragma once

#include <string>
#include <vector>

#include "common/wrappers/optional.h"

struct QueueTypesLeaseWorkerMode {
  int fields_mask{0};
  std::vector<int> queue_id;
  std::vector<std::string> type_names;

  static QueueTypesLeaseWorkerMode tl_fetch();
  void tl_store() const;
};

enum class LeaseWorkerMode {
  ALL_QUEUES, // default mode: get tasks from any queue
  QUEUE_TYPES // new worker mode: get tasks only from queues of given types with filter by queue_id
};

inline const char *lease_worker_mode_str(LeaseWorkerMode mode) {
  switch (mode) {
    case LeaseWorkerMode::ALL_QUEUES:
      return "LeaseWorkerMode::ALL_QUEUES";
    case LeaseWorkerMode::QUEUE_TYPES:
      return "LeaseWorkerMode::QUEUE_TYPES";
  }
  return "unreachable";
}

inline LeaseWorkerMode get_lease_mode(const vk::optional<QueueTypesLeaseWorkerMode> &mode) {
  return mode.has_value() ? LeaseWorkerMode::QUEUE_TYPES : LeaseWorkerMode::ALL_QUEUES;
}
