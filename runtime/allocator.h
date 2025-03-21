// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdlib>
#include <memory>

#include "common/containers/final_action.h"
#include "runtime-common/core/allocator/malloc-replacer.h"
#include "runtime-common/core/memory-resource/memory_resource.h"

namespace memory_resource {
class unsynchronized_pool_resource;
}

namespace dl {

extern bool script_allocator_enabled;
extern long long query_num; // engine query number. query_num == 0 before first query

memory_resource::unsynchronized_pool_resource &get_default_script_allocator() noexcept;

void set_current_script_allocator(memory_resource::unsynchronized_pool_resource &replacer, bool force_enable) noexcept;
void restore_default_script_allocator(bool force_disable) noexcept;

const memory_resource::MemoryStats &get_script_memory_stats() noexcept;
size_t get_heap_memory_used() noexcept;

void global_init_script_allocator() noexcept;
void init_script_allocator(void *buffer, size_t script_mem_size, size_t oom_handling_mem_size) noexcept;
void free_script_allocator() noexcept;

void *allocate(size_t n) noexcept; // allocate script memory
void *allocate0(size_t n) noexcept; // allocate zeroed script memory
void *reallocate(void *p, size_t new_size, size_t old_size) noexcept; // reallocate script memory
void deallocate(void *p, size_t n) noexcept; // deallocate script memory

void *heap_allocate(size_t n) noexcept; // allocate heap memory (persistent between script runs)
void *heap_reallocate(void *p, size_t new_size, size_t old_size) noexcept; // reallocate heap memory
void heap_deallocate(void *p, size_t n) noexcept; // deallocate heap memory

inline void *script_allocator_malloc(size_t size) noexcept {
  return kphp::malloc_replace::alloc(size);
}

inline void *script_allocator_calloc(size_t nmemb, size_t size) noexcept {
  return kphp::malloc_replace::calloc(nmemb, size);
}

inline void *script_allocator_realloc(void *p, size_t x) noexcept {
  return kphp::malloc_replace::realloc(p, x);
}

inline char *script_allocator_strdup(const char *str) noexcept {
  const size_t len = strlen(str) + 1;
  char *res = static_cast<char *>(script_allocator_malloc(len));
  if (unlikely(res == nullptr)) {
    return nullptr;
  }
  memcpy(res, str, len);
  return res;
}

inline void script_allocator_free(void *p) noexcept {
  kphp::malloc_replace::free(p);
}

bool is_malloc_replaced() noexcept;
void replace_malloc_with_script_allocator() noexcept;
void rollback_malloc_replacement() noexcept;
void write_last_malloc_replacement_stacktrace(char *buf, size_t buf_size) noexcept;

class MemoryReplacementGuard {
  bool force_enable_disable_;

public:
  explicit MemoryReplacementGuard(memory_resource::unsynchronized_pool_resource &memory_resource, bool force_enable_disable = false);
  ~MemoryReplacementGuard();
};

} // namespace dl

// replace malloc so it starts to use a script memory
inline auto make_malloc_replacement_with_script_allocator(bool replace = true) noexcept {
  if (replace) {
    dl::replace_malloc_with_script_allocator();
  }
  return vk::finally([replace] {
    if (replace) {
      dl::rollback_malloc_replacement();
    }
  });
}

// if malloc was replaced, temporarily revert that replacement
inline auto temporary_rollback_malloc_replacement() noexcept {
  const bool is_replaced = dl::is_malloc_replaced();
  if (is_replaced) {
    dl::rollback_malloc_replacement();
  }
  return vk::finally([is_replaced] {
    if (is_replaced) {
      dl::replace_malloc_with_script_allocator();
    }
  });
}

class ManagedThroughDlAllocator {
public:
  static void *operator new(size_t size) noexcept {
    return dl::allocate(size);
  }

  static void *operator new(size_t, void *ptr) noexcept {
    return ptr;
  }

  static void operator delete(void *ptr, size_t size) noexcept {
    dl::deallocate(ptr, size);
  }

  static void *operator new[](size_t count) = delete;
  static void operator delete[](void *ptr, size_t sz) = delete;
  static void operator delete[](void *ptr) = delete;

protected:
  ~ManagedThroughDlAllocator() = default;
};

template<typename T, typename... Args>
inline auto make_unique_on_script_memory(Args &&...args) noexcept {
  static_assert(std::is_base_of<ManagedThroughDlAllocator, T>{}, "ManagedThroughDlAllocator should be base for T");
  return std::make_unique<T>(std::forward<Args>(args)...);
}
