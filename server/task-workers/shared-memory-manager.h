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

  static constexpr size_t MEMORY_SIZE = 1024 * 1024 * 1024;
  static constexpr size_t SLICE_PAYLOAD_SIZE = 1024 * 1023;
  static constexpr size_t SLICE_META_INFO_SIZE = 1024;
  static constexpr size_t SLICE_SIZE = SLICE_PAYLOAD_SIZE + SLICE_META_INFO_SIZE;
  static constexpr size_t TOTAL_SLICES_COUNT = MEMORY_SIZE / SLICE_SIZE;

  static_assert(SLICE_PAYLOAD_SIZE % 1024 == 0, "SLICE_PAYLOAD_SIZE must be multiple of 1KB");
  static_assert(MEMORY_SIZE % SLICE_SIZE == 0, "MEMORY_SIZE must be multiple of SLICE_SIZE");

  // must be called from master once during the global initialization
  void init();

  void *allocate_slice();

  void deallocate_slice(void *slice);

private:
  unsigned char *memory{nullptr};

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
    return memory + idx * SLICE_SIZE;
  };

  SliceMetaInfo &get_slice_meta_info(unsigned char *slice_ptr) const {
    return *reinterpret_cast<SliceMetaInfo *>(slice_ptr + SLICE_PAYLOAD_SIZE);
  };

};

} // namespace task_workers
