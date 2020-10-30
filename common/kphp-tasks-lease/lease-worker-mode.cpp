#include "common/kphp-tasks-lease/lease-worker-mode.h"

#include "common/tl/constants/kphp.h"
#include "common/tl/fetch.h"
#include "common/tl/parse.h"
#include "common/tl/store.h"
#include "common/wrappers/optional.h"

QueueTypesLeaseWorkerMode QueueTypesLeaseWorkerMode::tl_fetch() {
  QueueTypesLeaseWorkerMode mode;
  mode.fields_mask = tl_fetch_int();
  if (mode.fields_mask & vk::tl::kphp::queue_types_mode_fields_mask::queue_id) {
    vk::tl::fetch_vector(mode.queue_id);
  }
  vk::tl::fetch_vector(mode.type_names);
  return mode;
}

void QueueTypesLeaseWorkerMode::tl_store() const {
  tl_store_int(fields_mask);
  if (fields_mask & vk::tl::kphp::queue_types_mode_fields_mask::queue_id) {
    vk::tl::store_vector(queue_id);
  }
  vk::tl::store_vector(type_names);
}

LeaseWorkerMode get_lease_mode(const vk::optional<QueueTypesLeaseWorkerMode> &mode) {
    return mode.has_value() ? LeaseWorkerMode::QUEUE_TYPES : LeaseWorkerMode::ALL_QUEUES;
}
