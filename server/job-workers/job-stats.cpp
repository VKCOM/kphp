// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <new>
#include <sys/mman.h>

#include "runtime/memory_resource/extra-memory-pool.h"

#include "server/job-workers/job-stats.h"

namespace job_workers {

size_t JobStats::MemoryBufferStats::write_stats_to(stats_t *stats, const char *prefix, size_t buffer_size) const noexcept {
  const size_t acquired_buffers = acquired.load(std::memory_order_relaxed);
  const size_t released_buffers = released.load(std::memory_order_relaxed);
  const size_t memory_used = acquired_buffers > released_buffers ? (acquired_buffers - released_buffers) * buffer_size : 0;

  add_gauge_stat(stats, count, prefix, "buffers_reserved");
  add_gauge_stat(stats, acquire_fails, prefix, "buffer_acquire_fails");
  add_gauge_stat(stats, acquired_buffers, prefix, "buffers_acquired");
  add_gauge_stat(stats, released_buffers, prefix, "buffers_released");

  add_gauge_stat(stats, memory_used, prefix, "currently_used_bytes");
  add_gauge_stat(stats, count * buffer_size, prefix, "reserved_bytes");

  return memory_used;
}

void JobStats::write_stats_to(stats_t *stats) const noexcept {
  const char *prefix = "job_workers.";
  add_gauge_stat(stats, errors_pipe_server_write, prefix, "errors_pipe_server_write");
  add_gauge_stat(stats, errors_pipe_server_read, prefix, "errors_pipe_server_read");
  add_gauge_stat(stats, errors_pipe_client_write, prefix, "errors_pipe_client_write");
  add_gauge_stat(stats, errors_pipe_client_read, prefix, "errors_pipe_client_read");

  add_gauge_stat(stats, job_worker_skip_job_due_another_is_running, prefix, "job_worker_skip_job_due_another_is_running");
  add_gauge_stat(stats, job_worker_skip_job_due_overload, prefix, "job_worker_skip_job_due_overload");
  add_gauge_stat(stats, job_worker_skip_job_due_steal, prefix, "job_worker_skip_job_due_steal");

  add_gauge_stat(stats, job_queue_size, prefix, "job_queue_size");
  add_gauge_stat(stats, jobs_sent, prefix, "jobs_sent");
  add_gauge_stat(stats, jobs_replied, prefix, "jobs_replied");

  size_t currently_used = messages.write_stats_to(stats, "job_workers.memory.messages.", JOB_SHARED_MESSAGE_BYTES);
  constexpr std::array<const char *, JOB_EXTRA_MEMORY_BUFFER_BUCKETS> extra_memory_prefixes{
    "job_workers.memory.extra_buffers.1mb.",
    "job_workers.memory.extra_buffers.2mb.",
    "job_workers.memory.extra_buffers.4mb.",
    "job_workers.memory.extra_buffers.8mb.",
    "job_workers.memory.extra_buffers.16mb.",
    "job_workers.memory.extra_buffers.32mb.",
    "job_workers.memory.extra_buffers.64mb.",
  };
  for (size_t i = 0; i != JOB_EXTRA_MEMORY_BUFFER_BUCKETS; ++i) {
    const size_t buffer_size = memory_resource::extra_memory_raw_bucket::get_size_by_bucket(i);
    currently_used += extra_memory[i].write_stats_to(stats, extra_memory_prefixes[i], buffer_size);
  }

  add_gauge_stat(stats, memory_limit, prefix, "memory.reserved_bytes");
  add_gauge_stat(stats, currently_used, prefix, "memory.currently_used_bytes");
  add_gauge_stat(stats, unused_memory, prefix, "memory.unused_bytes");
}

} // namespace job_workers
