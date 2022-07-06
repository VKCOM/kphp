// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/kphp-tasks-lease/lease-worker-mode.h"

#include "common/tl/constants/kphp.h"
#include "common/tl/fetch.h"
#include "common/tl/parse.h"
#include "common/tl/store.h"

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

QueueTypesLeaseWorkerModeV2 QueueTypesLeaseWorkerModeV2::tl_fetch() {
  QueueTypesLeaseWorkerModeV2 mode;
  assert(tl_fetch_int() == TL_KPHP_QUEUE_TYPES_MODE_V2);
  mode.fields_mask = tl_fetch_int();
  if (mode.fields_mask & vk::tl::kphp::queue_types_mode_v2_fields_mask::queue_id) {
    vk::tl::fetch_vector(mode.queue_id);
  }
  vk::tl::fetch_vector(mode.type_names);
  return mode;
}

void QueueTypesLeaseWorkerModeV2::tl_store() const {
  tl_store_int(TL_KPHP_QUEUE_TYPES_MODE_V2);
  tl_store_int(fields_mask);
  if (fields_mask & vk::tl::kphp::queue_types_mode_v2_fields_mask::queue_id) {
    vk::tl::store_vector(queue_id);
  }
  vk::tl::store_vector(type_names);
}
