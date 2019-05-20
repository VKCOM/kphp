#pragma once
#include "runtime/memory_resource/memory_resource.h"

#include "common/wrappers/likely.h"
#include "runtime/php_assert.h"

#include <algorithm>

namespace memory_resource {
class monotonic_buffer_resource {
public:
  void init(void *buffer, size_type buffer_size);

  void *allocate(size_type size) {
    void *mem = get_from_pool(size);
    memory_debug("allocate %d, allocated address from pool %p\n", size, mem);
    register_allocation(mem, size);
    return mem;
  }

  void *reallocate(void *mem, size_type new_size, size_type old_size) {
    return universal_reallocate(*this, mem, new_size, old_size);
  }

  void *try_expand(void *mem, size_type new_size, size_type old_size) {
    if (static_cast<char *>(mem) + old_size == memory_current_) {
      const auto additional_size = old_size - new_size;
      if (memory_end_ - memory_current_ >= additional_size) {
        memory_current_ += additional_size;
        register_allocation(mem, additional_size);
        return mem;
      }
    }
    return nullptr;
  }

  void deallocate(void *mem, size_type size) {
    memory_debug("deallocate %d at %p\n", size, mem);
    if (put_memory_back(mem, size)) {
      stats_.memory_used -= size;
      stats_.real_memory_used -= size;
    }
  }

  const MemoryStats &get_memory_stats() const noexcept { return stats_; }

protected:
  void critical_dump(void *mem, size_t size);

  void check_memory_piece(void *mem, size_t size) {
    if (unlikely(memory_begin_ > static_cast<char *>(mem) ||
                 static_cast<char *>(mem) + size > memory_end_)) {
      critical_dump(mem, size);
    }
  }

  bool put_memory_back(void *mem, size_type size) {
    check_memory_piece(mem, size);
    const bool expandable = static_cast<char *>(mem) + size == memory_current_;
    if (expandable) {
      memory_current_ = static_cast<char *>(mem);
    }
    return expandable;
  }

  void *get_from_pool(size_type size) {
    if (unlikely(memory_end_ - memory_current_ < size)) {
      php_warning("Can't allocate %u bytes", size);
      raise(SIGUSR2);
      return nullptr;
    }

    void *mem = memory_current_;
    memory_current_ += size;
    return mem;
  }

  void register_allocation(void *mem, size_type size) {
    if (mem) {
      stats_.memory_used += size;
      stats_.max_memory_used = std::max(stats_.max_memory_used, stats_.memory_used);
      stats_.real_memory_used = static_cast<size_type>(memory_current_ - memory_begin_);
      stats_.max_real_memory_used = std::max(stats_.real_memory_used, stats_.max_real_memory_used);
    }
  }

  MemoryStats stats_;

  static_assert(sizeof(char) == 1, "sizeof char should be 1");
  char *memory_begin_{nullptr};
  char *memory_current_{nullptr};
  char *memory_end_{nullptr};
};
} // namespace memory_resource
