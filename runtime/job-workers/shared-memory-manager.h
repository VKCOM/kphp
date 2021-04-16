// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"

#include "runtime/memory_resource/extra-memory-pool.h"

namespace job_workers {

struct JobSharedMessage;

class SharedMemoryManager : vk::not_copyable {
public:
  void init() noexcept;

  JobSharedMessage *acquire_shared_message() noexcept;
  void release_shared_message(JobSharedMessage *message) noexcept;

  void attach_shared_message_to_this_proc(JobSharedMessage *message) noexcept;
  void detach_shared_message_from_this_proc(JobSharedMessage *message) noexcept;

  void forcibly_release_all_attached_messages() noexcept;

  void set_memory_limit(size_t memory_limit) {
    memory_limit_ = memory_limit;
  }

  size_t get_messages_count() const noexcept {
    return messages_count_;
  }

  bool request_extra_memory_for_resource(memory_resource::unsynchronized_pool_resource &resource, size_t required_size) noexcept;

private:
  SharedMemoryManager() = default;

  friend class vk::singleton<SharedMemoryManager>;

  size_t memory_limit_{0};
  size_t messages_count_{0};

  struct ControlBlock;
  ControlBlock *control_block_{nullptr};
};

inline bool request_extra_shared_memory(memory_resource::unsynchronized_pool_resource &resource, size_t required_size) noexcept {
  return vk::singleton<SharedMemoryManager>::get().request_extra_memory_for_resource(resource, required_size);
}

} // namespace job_workers
