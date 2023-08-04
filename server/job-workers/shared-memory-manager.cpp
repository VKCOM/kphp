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
    auto mul = per_process_memory_limit_ ? per_process_memory_limit_ : JOB_DEFAULT_MEMORY_LIMIT_PROCESS_MULTIPLIER;
    memory_limit_ = processes * mul + sizeof(ControlBlock);
  }
  auto *raw_mem = static_cast<uint8_t *>(mmap_shared(memory_limit_));
  const auto *raw_mem_start = raw_mem;
  const auto *raw_mem_finish = raw_mem + memory_limit_;

  control_block_ = new(raw_mem) ControlBlock{};
  raw_mem += sizeof(ControlBlock);
  const size_t left_memory = memory_limit_ - sizeof(ControlBlock);

  std::array<size_t, 1 + JOB_EXTRA_MEMORY_BUFFER_BUCKETS> shared_memory_group_buffers_counts;
  control_block_->stats.unused_memory = calc_shared_memory_buffers_distribution(left_memory, shared_memory_group_buffers_counts);

  const uint32_t messages_count = shared_memory_group_buffers_counts[0];
  assert(messages_count > 0);
  for (uint32_t i = 0; i != messages_count; ++i) {
    freelist_put(&control_block_->free_messages, raw_mem);
    raw_mem += sizeof(JobSharedMessage);
  }

  for (size_t i = 1; i < shared_memory_buffers_groups_.size(); ++i) {
    const auto &cur_g = shared_memory_buffers_groups_[i];
    const size_t cur_group_buffers_cnt = shared_memory_group_buffers_counts[i];
    for (int j = 0; j < cur_group_buffers_cnt; ++j) {
      freelist_put(&control_block_->free_extra_memory[i - 1], raw_mem);
      raw_mem += cur_g.buffer_size;
    }
    control_block_->stats.extra_memory[i - 1].count = cur_group_buffers_cnt;
  }

  assert(raw_mem_start <= raw_mem && raw_mem <= raw_mem_finish);

  control_block_->stats.memory_limit = memory_limit_;
  control_block_->stats.messages.count = messages_count;
}

bool SharedMemoryManager::set_memory_limit(size_t memory_limit) noexcept {
  if (memory_limit < sizeof(JobSharedMessage) + sizeof(ControlBlock)) {
    return false;
  }
  memory_limit_ = memory_limit;
  return true;
}

bool SharedMemoryManager::set_per_process_memory_limit(size_t per_process_memory_limit) noexcept {
  if (per_process_memory_limit < JOB_DEFAULT_MEMORY_LIMIT_PROCESS_MULTIPLIER) {
    return false;
  }
  per_process_memory_limit_ = per_process_memory_limit;
  return true;
}

bool SharedMemoryManager::set_shared_memory_distribution_weights(const std::array<double, 1 + JOB_EXTRA_MEMORY_BUFFER_BUCKETS> &weights) noexcept {
  if (weights[0] < 1e-6) {
    return false;
  }
  for (size_t i = 0; i < weights.size(); ++i) {
    shared_memory_buffers_groups_[i].weight = weights[i];
  }
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
      const int i = get_extra_shared_memory_buffers_group_idx(releasing_extra_memory->get_buffer_size());
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
    const size_t buffer_real_size = get_extra_shared_memory_buffer_size(i);
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

size_t SharedMemoryManager::calc_shared_memory_buffers_distribution(size_t mem_size, std::array<size_t, 1 + JOB_EXTRA_MEMORY_BUFFER_BUCKETS> &group_buffers_counts) const noexcept {
  auto calc_group_buffers_count = [&](const shared_memory_buffers_group_info &cur_g) -> size_t {
    double sum = 0;
    for (auto &g : shared_memory_buffers_groups_) {
      sum += g.weight;
    }
    const double ratio = cur_g.weight / sum;
    return static_cast<size_t>(std::floor(mem_size * ratio / cur_g.buffer_size));
  };

  size_t total_used_mem = 0;
  for (size_t i = 0; i < shared_memory_buffers_groups_.size(); ++i) {
    auto &g = shared_memory_buffers_groups_[i];
    const size_t buffers_cnt = calc_group_buffers_count(g);
    group_buffers_counts[i] = buffers_cnt;
    total_used_mem += g.buffer_size * buffers_cnt;
  }
  assert(total_used_mem <= mem_size);
  // distribute left memory starting with the largest buffers
  size_t unused_mem = mem_size - total_used_mem;
  for (size_t i = shared_memory_buffers_groups_.size(); i-- > 0; ) {
    auto &g = shared_memory_buffers_groups_[i];
    size_t cnt = unused_mem / g.buffer_size;
    group_buffers_counts[i] += cnt;
    unused_mem = unused_mem % g.buffer_size;
  }
  return unused_mem;
}

} // namespace job_workers
