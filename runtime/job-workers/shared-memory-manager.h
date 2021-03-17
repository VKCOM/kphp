// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <atomic>
#include <random>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"

namespace job_workers {

struct JobSharedMessage;

class SharedMemoryManager : vk::not_copyable {
public:
  void init() noexcept;

  JobSharedMessage *acquire_shared_message() noexcept;
  void release_shared_message(JobSharedMessage *slice) noexcept;

  void set_memory_limit(size_t memory_limit) {
    memory_limit_ = memory_limit;
  }

  size_t get_total_slices_count() const noexcept {
    return slices_count_;
  }

private:
  SharedMemoryManager() = default;

  friend class vk::singleton<SharedMemoryManager>;

  JobSharedMessage *get_message(size_t index) noexcept {
    return reinterpret_cast<JobSharedMessage *>(static_cast<uint8_t *>(memory_) + index * memory_slice_size_);
  }

  static constexpr size_t memory_slice_size_ = 1024 * 1024;

  size_t memory_limit_{1024 * 1024 * 1024};
  size_t slices_count_{0};
  void *memory_{nullptr};
  std::random_device rd_;
};

} // namespace job_workers
