// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"

#include "runtime/job-workers/job-interface.h"
#include "runtime/memory_resource/unsynchronized_pool_resource.h"
#include "runtime/kphp_core.h"

namespace job_workers {

struct JobSharedMessage : vk::not_copyable {
  memory_resource::unsynchronized_pool_resource resource;
  class_instance<SendingInstanceBase> instance;

  std::atomic<pid_t> owner_pid{0};

  int job_id{0};
  int job_result_fd_idx{-1};
  double script_execution_timeout{-1.0};
};

} // namespace job_workers
