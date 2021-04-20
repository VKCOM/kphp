// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <sys/mman.h>

#include "server/job-workers/job-stats.h"
#include "server/job-workers/job-workers-context.h"
#include "server/php-engine-vars.h"

#include "runtime/critical_section.h"
#include "runtime/job-workers/job-message.h"

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

  const size_t processes_count = vk::singleton<job_workers::JobWorkersContext>::get().job_workers_num + std::max(workers_n, 1);
  if (!memory_limit_) {
    memory_limit_ = processes_count * 8 * 1024 * 1024;
  }
  auto *raw_mem = static_cast<uint8_t *>(mmap(nullptr, sizeof(SharedMemoryProcessOwnersTable) + memory_limit_,
                                              PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
  assert(raw_mem);
  owners_table_ = new(raw_mem) SharedMemoryProcessOwnersTable{};
  raw_mem += sizeof(SharedMemoryProcessOwnersTable);

  size_t left_memory = memory_limit_;
  const size_t messages_count_limit = 2 * (vk::singleton<job_workers::JobWorkersContext>::get().job_workers_num + std::max(workers_n, 1));
  messages_count_ = std::min(messages_count_limit, left_memory / sizeof(JobSharedMessage));
  messages_ = new(raw_mem) JobSharedMessage[messages_count_];
}

JobSharedMessage *SharedMemoryManager::acquire_shared_message() noexcept {
  assert(messages_);
  const size_t random_slice_idx = rd_() % messages_count_;
  size_t i = random_slice_idx;

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
  } while (i != random_slice_idx);
  return nullptr;
}

void SharedMemoryManager::release_shared_message(JobSharedMessage *message) noexcept {
  dl::CriticalSectionGuard critical_section;
  owners_table_->workers_table[logname_id].detach(message);
  if (--message->owners_counter == 0) {
    hard_reset_var(message->instance);
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

} // namespace job_workers
