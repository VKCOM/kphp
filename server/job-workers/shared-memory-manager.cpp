// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/wrappers/memory-utils.h"

#include "server/php-engine-vars.h"
#include "server/workers-control.h"

#include "runtime/critical_section.h"

#include "server/job-workers/shared-memory-manager.h"

namespace job_workers {

void SharedMemoryManager::init() noexcept {
  static_assert(sizeof(ControlBlock) % 8 == 0, "check yourself");
  assert(!control_block_);

  const size_t processes = vk::singleton<WorkersControl>::get().get_total_workers_count();
  assert(processes);
  if (!memory_limit_) {
    memory_limit_ = processes * JOB_DEFAULT_MEMORY_LIMIT_PROCESS_MULTIPLIER + sizeof(ControlBlock);
  }
  auto *raw_mem = static_cast<uint8_t *>(mmap_shared(memory_limit_));
  const size_t left_memory = memory_limit_ - sizeof(ControlBlock);
  const uint32_t messages_count = std::min(JOB_SHARED_MESSAGES_COUNT_PROCESS_MULTIPLIER * processes, left_memory / sizeof(JobSharedMessage));
  control_block_ = new(raw_mem) ControlBlock{};
  raw_mem += sizeof(ControlBlock);
  for (uint32_t i = 0; i != messages_count; ++i) {
    freelist_put(&control_block_->free_messages, raw_mem);
    raw_mem += sizeof(JobSharedMessage);
  }

  std::array<memory_resource::extra_memory_raw_bucket, JOB_EXTRA_MEMORY_BUFFER_BUCKETS> extra_memory;
  control_block_->stats.unused_memory = memory_resource::distribute_memory(extra_memory, processes, raw_mem,
                                                                           left_memory - sizeof(JobSharedMessage) * messages_count);
  for (size_t i = 0; i != extra_memory.size(); ++i) {
    auto &free_extra_buffers = control_block_->free_extra_memory[i];
    auto &extra_buffers = extra_memory[i];
    for (size_t mem_index = 0; mem_index != extra_buffers.buffers_count(); ++mem_index) {
      freelist_put(&free_extra_buffers, extra_buffers.get_extra_pool_raw(mem_index));
    }
    control_block_->stats.extra_memory[i].count = extra_buffers.buffers_count();
  }

  control_block_->stats.memory_limit = memory_limit_;
  control_block_->stats.messages.count = messages_count;
}

bool SharedMemoryManager::set_memory_limit(size_t memory_limit) noexcept {
  if (memory_limit < 3 * sizeof(JobSharedMessage) + sizeof(ControlBlock)) {
    return false;
  }
  memory_limit_ = memory_limit;
  return true;
}

void SharedMemoryManager::release_shared_message(JobMetadata *message) noexcept {
  dl::CriticalSectionGuard critical_section;
  control_block_->workers_table[logname_id].detach(message);
  if (--message->owners_counter == 0) {
    auto *common_job = message->get_common_job();
    if (common_job) {
      release_shared_message(common_job);
    }
    auto *extra_memory = message->resource.get_extra_memory_head();
    while (extra_memory->get_pool_payload_size()) {
      auto *releasing_extra_memory = extra_memory;
      extra_memory = extra_memory->next_in_chain;
      const int i = memory_resource::extra_memory_raw_bucket::get_bucket(*releasing_extra_memory);
      assert(i >= 0 && i < control_block_->free_extra_memory.size());
      freelist_put(&control_block_->free_extra_memory[i], releasing_extra_memory);
      ++control_block_->stats.extra_memory[i].released;
    }
    freelist_put(&control_block_->free_messages, message);
    ++control_block_->stats.messages.released;
  }
}

void SharedMemoryManager::attach_shared_message_to_this_proc(JobMetadata *message) noexcept {
  assert(message->owners_counter);
  dl::CriticalSectionGuard critical_section;
  control_block_->workers_table[logname_id].attach(message);
}

void SharedMemoryManager::detach_shared_message_from_this_proc(JobMetadata *message) noexcept {
  // Don't assert counter, because memory may be already freed
  dl::CriticalSectionGuard critical_section;
  control_block_->workers_table[logname_id].detach(message);
}

void SharedMemoryManager::forcibly_release_all_attached_messages() noexcept {
  if (control_block_) {
    dl::CriticalSectionGuard critical_section;
    for (JobMetadata *message : control_block_->workers_table[logname_id].attached_messages) {
      if (message) {
        assert(message->owners_counter != 0);
        release_shared_message(message);
      }
    }
  }
}

bool SharedMemoryManager::request_extra_memory_for_resource(memory_resource::unsynchronized_pool_resource &resource, size_t required_size) noexcept {
  assert(control_block_);
  for (size_t i = 0; i != control_block_->free_extra_memory.size(); ++i) {
    const size_t buffer_real_size = memory_resource::extra_memory_raw_bucket::get_size_by_bucket(i);
    const size_t payload_size = memory_resource::extra_memory_pool::get_pool_payload_size(buffer_real_size);
    if (payload_size < required_size) {
      continue;
    }

    auto &free_extra_buffers = control_block_->free_extra_memory[i];
    dl::CriticalSectionGuard critical_section;
    if (auto *extra_mem = freelist_get(&free_extra_buffers)) {
      resource.add_extra_memory(new(extra_mem) memory_resource::extra_memory_pool{buffer_real_size});
      ++control_block_->stats.extra_memory[i].acquired;
      return true;
    }
    ++control_block_->stats.extra_memory[i].acquire_fails;
  }
  return false;
}

JobStats &SharedMemoryManager::get_stats() noexcept {
  assert(control_block_);
  return control_block_->stats;
}

} // namespace job_workers
