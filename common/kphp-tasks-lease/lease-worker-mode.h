#pragma once

#include <string>
#include <vector>

namespace tl {
    template <class T> class optional;
}
namespace vk {
    using ::tl::optional;
}

// This mode is set from YAML config on KPHP server startup
// and sent from KPHP to tasks in kphp.readyV2

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

LeaseWorkerMode get_lease_mode(const vk::optional<QueueTypesLeaseWorkerMode> &mode);
