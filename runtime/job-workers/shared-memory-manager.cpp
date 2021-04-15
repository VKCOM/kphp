// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <sys/mman.h>

#include "server/job-workers/job-stats.h"
#include "server/job-workers/job-workers-context.h"
#include "server/php-engine-vars.h"

#include "runtime/critical_section.h"

#include "runtime/job-workers/shared-memory-manager.h"

namespace job_workers {

struct WorkerProcessMeta {
  // We can use only 2 messages at once for one worker process:
  //  for HTTP/RPC workers: one mutable request data, one immutable request data
  //  for Job workers: one for request, one for response
  // TODO use more?
  std::array<JobSharedMessage *, 2> attached_messages{};

  void attach(JobSharedMessage *message) noexcept {
    replace(nullptr, message);
  }

  void detach(JobSharedMessage *message) noexcept {
    replace(message, nullptr);
  }

private:
  void replace(JobSharedMessage *old_message, JobSharedMessage *new_message) noexcept {
    for (auto &message : attached_messages) {
      if (message == old_message) {
        message = new_message;
        return;
      }
    }
    assert(false);
  }
};

struct alignas(8) SharedMemoryProcessOwnersTable {
  std::array<WorkerProcessMeta, MAX_WORKERS> workers_table{};
};

static_assert(sizeof(SharedMemoryProcessOwnersTable) % 8 == 0, "check yourself");

void SharedMemoryManager::init() noexcept {
  assert(!messages_);
  assert(!owners_table_);

  const size_t processes = vk::singleton<job_workers::JobWorkersContext>::get().job_workers_num + std::max(workers_n, 1);
  if (!memory_limit_) {
    memory_limit_ = processes * JOB_DEFAULT_MEMORY_LIMIT_PROCESS_MULTIPLIER + sizeof(SharedMemoryProcessOwnersTable);
  }
  auto *raw_mem = static_cast<uint8_t *>(mmap(nullptr, memory_limit_,
                                              PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
  assert(raw_mem);
  owners_table_ = new(raw_mem) SharedMemoryProcessOwnersTable{};
  raw_mem += sizeof(SharedMemoryProcessOwnersTable);
  size_t left_memory = memory_limit_ - sizeof(SharedMemoryProcessOwnersTable);

  const size_t messages_count_limit = JOB_SHARED_MESSAGES_COUNT_PROCESS_MULTIPLIER *
                                      (vk::singleton<job_workers::JobWorkersContext>::get().job_workers_num + std::max(workers_n, 1));
  messages_count_ = std::min(messages_count_limit, left_memory / sizeof(JobSharedMessage));
  messages_ = new(raw_mem) JobSharedMessage[messages_count_];
  raw_mem += sizeof(JobSharedMessage) * messages_count_;
  left_memory -= sizeof(JobSharedMessage) * messages_count_;
  memory_resource::distribute_memory(extra_memory_, processes, raw_mem, left_memory);
}

JobSharedMessage *SharedMemoryManager::acquire_shared_message() noexcept {
  assert(messages_);
  const size_t random_index = rd_() % messages_count_;
  size_t i = random_index;

  do {
    auto *message = messages_ + i;
    uint32_t expected_ref_cnt = JOB_SHARED_MESSAGE_FREE_CNT;
    {
      dl::CriticalSectionGuard critical_section;
      if (message->owners_counter.compare_exchange_strong(expected_ref_cnt, 1)) {
        owners_table_->workers_table[logname_id].attach(message);
        ++JobStats::get().currently_messages_acquired;
        return message;
      } else {
        ++JobStats::get().message_acquire_false_iterations;
      }
    }

    if (++i == messages_count_) {
      i = 0;
    }
  } while (i != random_index);
  return nullptr;
}

void SharedMemoryManager::release_shared_message(JobSharedMessage *message) noexcept {
  dl::CriticalSectionGuard critical_section;
  owners_table_->workers_table[logname_id].detach(message);
  if (--message->owners_counter == 0) {
    hard_reset_var(message->instance);
    auto *extra_memory = message->resource.get_extra_memory_head();
    while (extra_memory->get_pool_payload_size()) {
      auto *releasing_extra_memory = extra_memory;
      extra_memory = extra_memory->next_in_chain;
      std::atomic_thread_fence(std::memory_order_release);
      const bool result = __sync_bool_compare_and_swap(&releasing_extra_memory->next_in_chain, extra_memory, nullptr);
      assert(result);
    }
    message->resource.hard_reset();
    message->job_id = 0;
    message->job_result_fd_idx = -1;
    const auto owners_counter = message->owners_counter.exchange(JOB_SHARED_MESSAGE_FREE_CNT);
    assert(owners_counter == 0);
    --JobStats::get().currently_messages_acquired;
  }
}

void SharedMemoryManager::attach_shared_message_to_this_proc(JobSharedMessage *message) noexcept {
  assert(message->owners_counter);
  dl::CriticalSectionGuard critical_section;
  owners_table_->workers_table[logname_id].attach(message);
}

void SharedMemoryManager::detach_shared_message_from_this_proc(JobSharedMessage *message) noexcept {
  // Don't assert counter, because memory may be already freed
  dl::CriticalSectionGuard critical_section;
  owners_table_->workers_table[logname_id].detach(message);
}

void SharedMemoryManager::forcibly_release_all_attached_messages() noexcept {
  if (owners_table_) {
    dl::CriticalSectionGuard critical_section;
    for (JobSharedMessage *message : owners_table_->workers_table[logname_id].attached_messages) {
      if (message) {
        assert(message->owners_counter != 0);
        assert(message->owners_counter != JOB_SHARED_MESSAGE_FREE_CNT);
        release_shared_message(message);
      }
    }
  }
}

bool SharedMemoryManager::request_extra_memory_for_resource(memory_resource::unsynchronized_pool_resource &resource, size_t required_size) noexcept {
  for (auto &extra_buffers: extra_memory_) {
    const size_t total_buffers = extra_buffers.buffers_count();
    if (extra_buffers.get_buffer_payload_size() < required_size || total_buffers == 0) {
      continue;
    }

    // TODO random is not the best choice?
    const size_t random_index = rd_() % total_buffers;
    size_t i = random_index;
    do {
      {
        dl::CriticalSectionGuard critical_section;
        memory_resource::extra_memory_pool *extra_pool = extra_buffers.get_extra_pool(i);
        if (__sync_bool_compare_and_swap(&extra_pool->next_in_chain, nullptr,
                                         resource.get_extra_memory_head())) {
          std::atomic_thread_fence(std::memory_order_acquire);
          resource.add_extra_memory(extra_pool);
          return true;
        }
      }

      if (++i == total_buffers) {
        i = 0;
      }
    } while (i != random_index);
  }
  return false;
}

} // namespace job_workers
