#pragma once

#include <cstddef>
#include <cstdlib>
#include <memory>

#include "common/containers/final_action.h"
#include "runtime/memory_resource/memory_resource.h"

namespace memory_resource {

class synchronized_pool_resource;
class unsynchronized_pool_resource;

}

namespace dl {

extern long long query_num; // engine query number. query_num == 0 before first query

using size_type = memory_resource::size_type;

void set_script_allocator_replacement(memory_resource::synchronized_pool_resource &replacer) noexcept;
void drop_script_allocator_replacement() noexcept;

void set_current_script_allocator_and_enable_it(memory_resource::unsynchronized_pool_resource &replacer) noexcept;
void restore_current_script_allocator_and_disable_it() noexcept;

const memory_resource::MemoryStats &get_script_memory_stats() noexcept;
size_type get_heap_memory_used() noexcept;

void global_init_script_allocator() noexcept;
void init_script_allocator(void *buffer, size_type buffer_size) noexcept; // init script allocator with arena of n bytes at buf
void free_script_allocator() noexcept;

void *allocate(size_type n) noexcept; // allocate script memory
void *allocate0(size_type n) noexcept; // allocate zeroed script memory
void *reallocate(void *p, size_type new_size, size_type old_size) noexcept; // reallocate script memory
void deallocate(void *p, size_type n) noexcept; // deallocate script memory

void perform_script_allocator_defragmentation() noexcept;

void *heap_allocate(size_type n) noexcept; // allocate heap memory (persistent between script runs)
void *heap_reallocate(void *p, size_type new_size, size_type old_size) noexcept; // reallocate heap memory
void heap_deallocate(void *p, size_type n) noexcept; // deallocate heap memory

void *script_allocator_malloc(size_t x) noexcept;
void *script_allocator_calloc(size_t nmemb, size_t size) noexcept;
void *script_allocator_realloc(void *p, size_t x) noexcept;
char *script_allocator_strdup(const char *str) noexcept;
void script_allocator_free(void *p) noexcept;

bool is_malloc_replaced() noexcept;
void replace_malloc_with_script_allocator() noexcept;
void rollback_malloc_replacement() noexcept;

} // namespace dl

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

class ManagedThroughDlAllocator {
public:
  void *operator new(size_t size) noexcept {
    return dl::allocate(size);
  }

  void *operator new(size_t, void *ptr) noexcept {
    return ptr;
  }

  void operator delete(void *ptr, size_t size) noexcept {
    dl::deallocate(ptr, size);
  }

  void *operator new[](size_t count) = delete;
  void operator delete[](void *ptr, size_t sz) = delete;
  void operator delete[](void *ptr) = delete;

protected:
  ~ManagedThroughDlAllocator() = default;
};

template<typename T, typename... Args>
inline auto make_unique_on_script_memory(Args &&... args) noexcept {
  static_assert(std::is_base_of<ManagedThroughDlAllocator, T>{}, "ManagedThroughDlAllocator should be base for T");
  return std::make_unique<T>(std::forward<Args>(args)...);
}
