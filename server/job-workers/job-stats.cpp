// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <new>
#include <sys/mman.h>

#include "runtime-common/core/memory-resource/extra-memory-pool.h"

#include "server/job-workers/job-stats.h"

namespace job_workers {

size_t JobStats::MemoryBufferStats::write_stats_to(stats_t *stats, const char *prefix, size_t buffer_size) const noexcept {
  const size_t acquired_buffers = acquired.load(std::memory_order_relaxed);
  const size_t released_buffers = released.load(std::memory_order_relaxed);
  const size_t memory_used = acquired_buffers > released_buffers ? (acquired_buffers - released_buffers) * buffer_size : 0;

  stats->add_gauge_stat(count, prefix, "buffers_reserved");
  stats->add_gauge_stat(acquire_fails, prefix, "buffer_acquire_fails");
  stats->add_gauge_stat(acquired_buffers, prefix, "buffers_acquired");
  stats->add_gauge_stat(released_buffers, prefix, "buffers_released");

  stats->add_gauge_stat(memory_used, prefix, "currently_used_bytes");
  stats->add_gauge_stat(count * buffer_size, prefix, "reserved_bytes");

  return memory_used;
}

void JobStats::write_stats_to(stats_t *stats) const noexcept {
  const char *prefix = "workers.job.";
  stats->add_gauge_stat(errors_pipe_server_write, prefix, "pipe_errors.server_write");
  stats->add_gauge_stat(errors_pipe_server_read, prefix, "pipe_errors.server_read");
  stats->add_gauge_stat(errors_pipe_client_write, prefix, "pipe_errors.client_write");
  stats->add_gauge_stat(errors_pipe_client_read, prefix, "pipe_errors.client_read");

  stats->add_gauge_stat(job_worker_skip_job_due_another_is_running, prefix, "jobs.skip.another_is_running");
  stats->add_gauge_stat(job_worker_skip_job_due_steal, prefix, "jobs.skip.steal");

  stats->add_gauge_stat(job_queue_size, prefix, "jobs.queue_size");
  stats->add_gauge_stat(jobs_sent, prefix, "jobs.sent");
  stats->add_gauge_stat(jobs_replied, prefix, "jobs.replied");

  size_t currently_used = messages.write_stats_to(stats, "workers.job.memory.messages.shared_messages.", JOB_SHARED_MESSAGE_BYTES);
  constexpr std::array<const char *, JOB_EXTRA_MEMORY_BUFFER_BUCKETS> extra_memory_prefixes{
    "workers.job.memory.messages.extra_buffers.256kb.",
    "workers.job.memory.messages.extra_buffers.512kb.",
    "workers.job.memory.messages.extra_buffers.1mb.",
    "workers.job.memory.messages.extra_buffers.2mb.",
    "workers.job.memory.messages.extra_buffers.4mb.",
    "workers.job.memory.messages.extra_buffers.8mb.",
    "workers.job.memory.messages.extra_buffers.16mb.",
    "workers.job.memory.messages.extra_buffers.32mb.",
    "workers.job.memory.messages.extra_buffers.64mb.",
  };
  for (size_t i = 0; i != JOB_EXTRA_MEMORY_BUFFER_BUCKETS; ++i) {
    const size_t buffer_size = get_extra_shared_memory_buffer_size(i);
    currently_used += extra_memory[i].write_stats_to(stats, extra_memory_prefixes[i], buffer_size);
  }

  stats->add_gauge_stat(memory_limit, prefix, "memory.messages.reserved_bytes");
  stats->add_gauge_stat(currently_used, prefix, "memory.messages.currently_used_bytes");
  stats->add_gauge_stat(unused_memory, prefix, "memory.messages.unused_bytes");
}

} // namespace job_workers
