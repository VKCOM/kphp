// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <atomic>
#include <sys/types.h>
#include <unistd.h>

#include "common/smart_ptrs/singleton.h"
#include "common/mixin/not_copyable.h"
#include "runtime/inter-process-mutex.h"

namespace task_workers {

class SharedMemoryManager : vk::not_copyable {
public:
  friend class vk::singleton<SharedMemoryManager>;

  void set_memory_size(size_t memory_size_);
  void set_slice_payload_size(size_t slice_payload_size_);

  size_t get_slice_payload_size() const;
  size_t get_slice_meta_info_size() const;
  size_t get_total_slices_count() const;

  // must be called from master once during the global initialization
  void init();

  void *allocate_slice();

  void deallocate_slice(void *slice);

private:
  unsigned char *memory{nullptr};
  size_t memory_size = 1024 * 1024 * 1024;
  size_t slice_payload_size = 1024 * 1023;
  const size_t slice_meta_info_size = 1024;

  size_t slice_size{};
  size_t total_slices_count{};

  struct SliceMetaInfo {
    std::atomic<pid_t> owner_pid{0};

    bool acquire() {
      pid_t old_pid = 0;
      return owner_pid.compare_exchange_strong(old_pid, getpid());
    }

    bool release() {
      pid_t old_pid = owner_pid.exchange(0);
      return old_pid != 0;
    }

    bool is_vacant() const {
      return owner_pid == 0;
    }
  };

  SharedMemoryManager() = default;

  unsigned char *get_slice_ptr(size_t idx) const {
    return memory + idx * slice_size;
  };

  SliceMetaInfo &get_slice_meta_info(unsigned char *slice_ptr) const {
    return *reinterpret_cast<SliceMetaInfo *>(slice_ptr + slice_payload_size);
  };

};

} // namespace task_workers
