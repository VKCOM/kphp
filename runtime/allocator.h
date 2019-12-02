#pragma once

#include <cstddef>
#include <cstdlib>
#include <memory>

#include "runtime/memory_resource/memory_resource.h"

namespace memory_resource {

class synchronized_pool_resource;

}

namespace dl {

extern long long query_num; // engine query number. query_num == 0 before first query
extern volatile bool script_runned; // betwen init_static and free_static
extern volatile bool replace_malloc_with_script_allocator; // replace malloc and cpp operator new with script allocator

using size_type = memory_resource::size_type;

void set_script_allocator_replacement(memory_resource::synchronized_pool_resource *replacer);
void drop_script_allocator_replacement();
const memory_resource::MemoryStats &get_script_memory_stats();
size_type get_heap_memory_used();

void global_init_script_allocator();
void init_script_allocator(void *buffer, size_type buffer_size); // init script allocator with arena of n bytes at buf
void free_script_allocator();

void *allocate(size_type n); // allocate script memory
void *allocate0(size_type n); // allocate zeroed script memory
void *reallocate(void *p, size_type new_size, size_type old_size); // reallocate script memory
void deallocate(void *p, size_type n); // deallocate script memory

void *heap_allocate(size_type n); // allocate heap memory (persistent between script runs)
void *heap_reallocate(void *p, size_type new_size, size_type old_size); // reallocate heap memory
void heap_deallocate(void *p, size_type n); // deallocate heap memory

void *script_allocator_malloc(size_t x) noexcept;
void *script_allocator_calloc(size_t nmemb, size_t size) noexcept;
void *script_allocator_realloc(void *p, size_t x) noexcept;
char *script_allocator_strdup(const char *str) noexcept;
void script_allocator_free(void *p) noexcept;

void *replaceable_malloc(size_t x) noexcept;
void replaceable_free(void *p) noexcept;

} // namespace dl

class ManagedThroughDlAllocator {
public:
  void *operator new(size_t size) {
    return dl::allocate(size);
  }

  void *operator new(size_t, void *ptr) {
    return ptr;
  }

  void operator delete(void *ptr, size_t size) {
    dl::deallocate(ptr, size);
  }

  void *operator new[](size_t count) = delete;
  void operator delete[](void *ptr, size_t sz) = delete;
  void operator delete[](void *ptr) = delete;

protected:
  ~ManagedThroughDlAllocator() = default;
};

template<typename T, typename... Args>
inline auto make_unique_on_script_memory(Args &&... args) {
  static_assert(std::is_base_of<ManagedThroughDlAllocator, T>{}, "ManagedThroughDlAllocator should be base for T");
  return std::make_unique<T>(std::forward<Args>(args)...);
}
