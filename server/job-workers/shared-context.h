// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <atomic>
#include <cstddef>

#include "common/mixin/not_copyable.h"

namespace job_workers {

class SharedContext : vk::not_copyable {
public:
  std::atomic<int> job_queue_size{0};
  std::atomic<int> occupied_slices_count{0};
  std::atomic<size_t> total_jobs_sent{0};
  std::atomic<size_t> total_jobs_done{0};
  std::atomic<size_t> total_jobs_failed{0};

  std::atomic<size_t> total_errors_pipe_server_write{0};
  std::atomic<size_t> total_errors_pipe_server_read{0};
  std::atomic<size_t> total_errors_pipe_client_write{0};
  std::atomic<size_t> total_errors_pipe_client_read{0};
  std::atomic<size_t> total_errors_shared_memory_limit{0};

  static SharedContext &make();

  // must be called from master process on global init
  static SharedContext &get();

private:
  SharedContext() = default;
};

} // namespace job_workers
