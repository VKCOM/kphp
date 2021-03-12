// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <atomic>
#include <random>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"

#include "runtime/job-workers/job-interface.h"
#include "runtime/kphp_core.h"
#include "runtime/memory_resource/unsynchronized_pool_resource.h"

namespace job_workers {

struct SharedMemorySlice : vk::not_copyable {
  memory_resource::unsynchronized_pool_resource resource;
  std::atomic<pid_t> owner_pid{0};
  class_instance<SendingInstanceBase> instance;
};

class SharedMemoryManager : vk::not_copyable {
public:
  void init() noexcept;

  SharedMemorySlice *acquire_slice() noexcept;
  void release_slice(SharedMemorySlice *slice) noexcept;

  void set_memory_limit(size_t memory_limit) {
    memory_limit_ = memory_limit;
  }

  size_t get_total_slices_count() const noexcept {
    return slices_count_;
  }

private:
  SharedMemoryManager() = default;

  friend class vk::singleton<SharedMemoryManager>;

  SharedMemorySlice *get_slice(size_t index) noexcept {
    return reinterpret_cast<SharedMemorySlice *>(static_cast<uint8_t *>(memory_) + index * memory_slice_size_);
  }

  static constexpr size_t memory_slice_size_ = 1024 * 1024;

  size_t memory_limit_{1024 * 1024 * 1024};
  size_t slices_count_{0};
  void *memory_{nullptr};
  std::random_device rd_;
};

} // namespace job_workers
