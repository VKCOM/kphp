// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <sys/mman.h>

#include "server/job-workers/shared-context.h"
#include "server/php-engine-vars.h"

#include "runtime/job-workers/shared-memory-manager.h"

namespace job_workers {

void SharedMemoryManager::init() noexcept {
  constexpr size_t control_block_size = (sizeof(SharedMemorySlice) + 7) & -8;
  static_assert(memory_slice_size_ > control_block_size, "too huge control block");
  slices_count_ = memory_limit_ / memory_slice_size_;
  memory_ = mmap(nullptr, memory_limit_, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  for (size_t i = 0; i < slices_count_; ++i) {
    auto *slice = get_slice(i);
    slice = new(slice) SharedMemorySlice();
    slice->resource.init(reinterpret_cast<uint8_t *>(slice) + control_block_size, memory_slice_size_ - control_block_size);
  }
}

SharedMemorySlice *SharedMemoryManager::acquire_slice() noexcept {
  const size_t random_slice_idx = rd_() % slices_count_;
  size_t i = random_slice_idx;

  do {
    auto *slice = get_slice(i);
    pid_t prev_pid = 0;
    if (slice->owner_pid.compare_exchange_strong(prev_pid, pid)) {
      SharedContext::get().occupied_slices_count++;
      return slice;
    }

    if (++i == slices_count_) {
      i = 0;
    }
  } while (i != random_slice_idx);
  return nullptr;
}

void SharedMemoryManager::release_slice(SharedMemorySlice *slice) noexcept {
  hard_reset_var(slice->instance);
  slice->resource.init(slice->resource.memory_begin(), slice->resource.get_memory_stats().memory_limit);
  const pid_t prev_pid = slice->owner_pid.exchange(0);
  php_assert(prev_pid);
}

} // namespace job_workers
