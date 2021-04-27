// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"
#include "common/kphp-tasks-lease/lease-worker-mode.h"


class LeaseContext : vk::not_copyable {
public:
  vk::optional<QueueTypesLeaseWorkerMode> cur_lease_mode;
  double rpc_stop_ready_timeout{0};

  friend class vk::singleton<LeaseContext>;

private:
  LeaseContext() = default;
};
