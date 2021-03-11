// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <atomic>
#include <cstddef>

#include "common/mixin/not_copyable.h"

namespace job_workers {

class JobStats : vk::not_copyable {
public:
  std::atomic<int> job_queue_size{0};
  std::atomic<int> currently_memory_slices_used{0};
  std::atomic<size_t> jobs_sent{0};
  std::atomic<size_t> jobs_replied{0};

  std::atomic<size_t> errors_pipe_server_write{0};
  std::atomic<size_t> errors_pipe_server_read{0};
  std::atomic<size_t> errors_pipe_client_write{0};
  std::atomic<size_t> errors_pipe_client_read{0};

  std::atomic<size_t> job_worker_skip_job_due_another_is_running{0};
  std::atomic<size_t> job_worker_skip_job_due_overload{0};
  std::atomic<size_t> job_worker_skip_job_due_steal{0};

  // must be called from master process on global init
  static JobStats &get();

private:
  JobStats() = default;

  static JobStats &make();
};

} // namespace job_workers
