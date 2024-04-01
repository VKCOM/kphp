// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <csignal>

#include "common/mixin/not_copyable.h"
#include "common/wrappers/likely.h"

#include "runtime/memory_resource/details/universal_reallocate.h"
#include "runtime/memory_resource/memory_resource.h"
#include "runtime/php_assert.h"

namespace memory_resource {

class monotonic_buffer : vk::not_copyable {
protected:
  void init(void *buffer, size_t buffer_size) noexcept;

  size_t size() const noexcept {
    return static_cast<size_t>(memory_end_ - memory_current_);
  }
  void *memory_begin() const noexcept {
    return memory_begin_;
  }
  void *memory_current() const noexcept {
    return memory_current_;
  }
  const MemoryStats &get_memory_stats() const noexcept {
    return stats_;
  }

  void register_allocation(void *mem, size_t size) noexcept {
    if (mem) {
      stats_.memory_used += size;
      stats_.max_memory_used = std::max(stats_.max_memory_used, stats_.memory_used);
      stats_.real_memory_used = static_cast<size_t>(memory_current_ - memory_begin_);
      stats_.max_real_memory_used = std::max(stats_.real_memory_used, stats_.max_real_memory_used);
      ++stats_.total_allocations;
      stats_.total_memory_allocated += size;
    }
  }

  void register_deallocation(size_t size) noexcept {
    stats_.memory_used -= size;
    stats_.real_memory_used = static_cast<size_t>(memory_current_ - memory_begin_);
  }

  bool check_memory_piece(void *mem, size_t size) const noexcept {
    return memory_begin_ <= static_cast<char *>(mem) && static_cast<char *>(mem) + size <= memory_current_;
  }

  void critical_dump(void *mem, size_t size) const noexcept;

  MemoryStats stats_;

  static_assert(sizeof(char) == 1, "sizeof char should be 1");
  char *memory_current_{nullptr};
  char *memory_begin_{nullptr};
  char *memory_end_{nullptr};
};

class monotonic_buffer_resource : protected monotonic_buffer {
public:
  using monotonic_buffer::get_memory_stats;
  using monotonic_buffer::init;
  using monotonic_buffer::memory_begin;
  using monotonic_buffer::memory_current;
  using monotonic_buffer::size;

  void *allocate(size_t size) noexcept {
    void *mem = get_from_pool(size);
    memory_debug("allocate %zu, allocated address from pool %p\n", size, mem);
    register_allocation(mem, size);
    return mem;
  }

  void *allocate0(size_t size) noexcept {
    return memset(allocate(size), 0x00, size);
  }

  void *reallocate(void *mem, size_t new_size, size_t old_size) noexcept {
    return details::universal_reallocate(*this, mem, new_size, old_size);
  }

  void *try_expand(void *mem, size_t new_size, size_t old_size) noexcept {
    if (static_cast<char *>(mem) + old_size == memory_current_) {
      const auto additional_size = old_size - new_size;
      if (static_cast<size_t>(memory_end_ - memory_current_) >= additional_size) {
        memory_current_ += additional_size;
        register_allocation(mem, additional_size);
        return mem;
      }
    }
    return nullptr;
  }

  void deallocate(void *mem, size_t size) noexcept {
    memory_debug("deallocate %zu at %p\n", size, mem);
    if (put_memory_back(mem, size)) {
      register_deallocation(size);
    }
  }

  bool put_memory_back(void *mem, size_t size) noexcept {
    if (unlikely(!check_memory_piece(mem, size))) {
      critical_dump(mem, size);
    }

    const bool expandable = static_cast<char *>(mem) + size == memory_current_;
    if (expandable) {
      memory_current_ = static_cast<char *>(mem);
    }
    return expandable;
  }

  void *get_from_pool(size_t size, bool safe = false) noexcept {
    if (unlikely(static_cast<size_t>(memory_end_ - memory_current_) < size)) {
      if (unlikely(!safe)) {
        raise_oom(size);
      }
      return nullptr;
    }

    void *mem = memory_current_;
    memory_current_ += size;
    return mem;
  }

protected:
  // this function should never be called from the nested/base context,
  // since all allocators have their own mem stats;
  // when signaling OOM, we want to see the root mem stats, not the
  // fallback buffer mem stats, for instance
  void raise_oom(size_t size) const noexcept;
};

} // namespace memory_resource
