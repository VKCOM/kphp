// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <atomic>
#include <cstddef>

#include "common/mixin/not_copyable.h"
#include "common/stats/provider.h"

#include "server/job-workers/job-message.h"

namespace job_workers {

class JobStats : vk::not_copyable {
public:
  std::atomic<uint32_t> errors_pipe_server_write{0};
  std::atomic<uint32_t> errors_pipe_server_read{0};
  std::atomic<uint32_t> errors_pipe_client_write{0};
  std::atomic<uint32_t> errors_pipe_client_read{0};

  std::atomic<uint32_t> job_worker_skip_job_due_another_is_running{0};
  std::atomic<size_t> job_worker_skip_job_due_steal{0};
  std::atomic<uint32_t> job_worker_skip_job_due_timeout_expired{0};

  std::atomic<size_t> jobs_sent{0};
  std::atomic<size_t> jobs_replied{0};
  std::atomic<int32_t> job_queue_size{0};

  uint32_t unused_memory{0};
  size_t memory_limit{0};

  struct MemoryBufferStats : private vk::not_copyable {
    uint32_t count{0};
    std::atomic<uint32_t> acquire_fails{0};
    std::atomic<size_t> acquired{0};
    std::atomic<size_t> released{0};

    size_t write_stats_to(stats_t *stats, const char *prefix, size_t buffer_size) const noexcept;
  };

  MemoryBufferStats messages;
  std::array<MemoryBufferStats, JOB_EXTRA_MEMORY_BUFFER_BUCKETS> extra_memory{};

  void write_stats_to(stats_t *stats) const noexcept;
};

} // namespace job_workers
