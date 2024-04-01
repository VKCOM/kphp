// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <optional>

#include "common/kphp-tasks-lease/lease-worker-mode.h"
#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"

class LeaseContext : vk::not_copyable {
public:
  std::optional<QueueTypesLeaseWorkerMode> cur_lease_mode;
  std::optional<QueueTypesLeaseWorkerModeV2> cur_lease_mode_v2;
  double rpc_stop_ready_timeout{0};

  friend class vk::singleton<LeaseContext>;

private:
  LeaseContext() = default;
};
