// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/allocators/freelist.h"
#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"

#include "runtime/critical_section.h"
#include "runtime/memory_resource/extra-memory-pool.h"
#include "server/job-workers/job-stats.h"
#include "server/job-workers/job-workers-context.h"
#include "server/php-engine-vars.h"
#include "server/workers-control.h"

namespace memory_resource {
class unsynchronized_pool_resource;
} // namespace memory_resource

namespace job_workers {

class JobStats;

struct JobSharedMessage;

struct WorkerProcessMeta {
  // We can use only 4 messages at once for one worker process:
  //    mutable request data
  //    immutable request data
  //    ok response
  //    error response (only for job workers)
  // + 2 messages: mutable request & immutable request, if job is invoked from running job
  // so let's use 8 just in case
  std::array<JobMetadata *, 8> attached_messages{};

  void attach(JobMetadata *message) noexcept {
    replace(nullptr, message);
  }

  void detach(JobMetadata *message) noexcept {
    replace(message, nullptr);
  }

private:
  void replace(JobMetadata *old_message, JobMetadata *new_message) noexcept {
    for (auto &message : attached_messages) {
      if (message == old_message) {
        message = new_message;
        return;
      }
    }
    assert(false);
  }
};

class SharedMemoryManager : vk::not_copyable {
public:
  void init() noexcept;

  template <typename JobMessageT>
  JobMessageT *acquire_shared_message() noexcept {
    assert(control_block_);
    dl::CriticalSectionGuard critical_section;
    if (void *free_mem = freelist_get(&control_block_->free_messages)) {
      auto *message = new(free_mem) JobMessageT{};
      control_block_->workers_table[logname_id].attach(message);
      ++control_block_->stats.messages.acquired;
      return message;
    }
    ++control_block_->stats.messages.acquire_fails;
    return nullptr;
  }

  void release_shared_message(JobMetadata *message) noexcept;

  void attach_shared_message_to_this_proc(JobMetadata *message) noexcept;
  void detach_shared_message_from_this_proc(JobMetadata *message) noexcept;

  void forcibly_release_all_attached_messages() noexcept;

  bool set_memory_limit(size_t memory_limit) noexcept;
  bool set_per_process_memory_limit(size_t per_process_memory_limit) noexcept;
  bool set_shared_memory_distribution_weights(const std::array<double, 1 + JOB_EXTRA_MEMORY_BUFFER_BUCKETS> &weights) noexcept;

  bool request_extra_memory_for_resource(memory_resource::unsynchronized_pool_resource &resource, size_t required_size) noexcept;

  JobStats &get_stats() noexcept;

  bool is_initialized() const noexcept {
    return control_block_;
  }

  size_t calc_shared_memory_buffers_distribution(size_t mem_size, std::array<size_t, 1 + JOB_EXTRA_MEMORY_BUFFER_BUCKETS> &group_buffers_counts) const noexcept;

private:
  SharedMemoryManager() = default;

  friend class vk::singleton<SharedMemoryManager>;

  size_t memory_limit_{0};
  size_t per_process_memory_limit_{0};
  // weights for distributing shared memory between buffers groups
  // quantity of i-th memory piece is calculated like (w[i]/sum(w) * memory_limit_) / memory_piece_size
  struct shared_memory_buffers_group_info {
    size_t buffer_size;
    double weight;
  };
  std::array<shared_memory_buffers_group_info, 1 + JOB_EXTRA_MEMORY_BUFFER_BUCKETS> shared_memory_buffers_groups_{
    {
      {1 << JOB_SHARED_MESSAGE_SIZE_EXP,       2}, // 128KB messages
      // and extra buffers:
      {1 << (JOB_SHARED_MESSAGE_SIZE_EXP + 1), 2}, // 256KB
      {1 << (JOB_SHARED_MESSAGE_SIZE_EXP + 2), 2}, // 512KB
      {1 << (JOB_SHARED_MESSAGE_SIZE_EXP + 3), 2}, // 1MB
      {1 << (JOB_SHARED_MESSAGE_SIZE_EXP + 4), 1}, // 2MB
      {1 << (JOB_SHARED_MESSAGE_SIZE_EXP + 5), 1}, // 4MB
      {1 << (JOB_SHARED_MESSAGE_SIZE_EXP + 6), 1}, // 8MB
      {1 << (JOB_SHARED_MESSAGE_SIZE_EXP + 7), 1}, // 16MB
      {1 << (JOB_SHARED_MESSAGE_SIZE_EXP + 8), 1}, // 32MB
      {1 << (JOB_SHARED_MESSAGE_SIZE_EXP + 9), 1}, // 64MB
    }
  };

  struct alignas(8) ControlBlock {
    ControlBlock() noexcept {
      freelist_init(&free_messages);
      for (auto &free_mem : free_extra_memory) {
        freelist_init(&free_mem);
      }
    }

    JobStats stats;
    std::array<WorkerProcessMeta, WorkersControl::max_workers_count> workers_table{};
    freelist_t free_messages{};

    // 0 => 256KB, 1 => 512KB, 2 => 1MB, 3 => 2MB, 4 => 4MB, 5 => 8MB, 6 => 16MB, 7 => 32MB, 8 => 64MB
    std::array<freelist_t, JOB_EXTRA_MEMORY_BUFFER_BUCKETS> free_extra_memory{};
  };
  ControlBlock *control_block_{nullptr};
};

inline bool request_extra_shared_memory(memory_resource::unsynchronized_pool_resource &resource, size_t required_size) noexcept {
  return vk::singleton<SharedMemoryManager>::get().request_extra_memory_for_resource(resource, required_size);
}

} // namespace job_workers
