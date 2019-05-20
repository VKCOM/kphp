#pragma once
#include <cstdarg>
#include <cstdio>
#include <cstring>

// #define DEBUG_MEMORY;

inline void memory_debug(const char *format, ...) __attribute__ ((format (printf, 1, 2)));

inline void memory_debug(const char *format, ...) {
  (void)format;
#ifdef DEBUG_MEMORY
  va_list argptr;
  va_start(argptr, format);
  vfprintf(stderr, format, argptr);
  va_end(argptr);
#endif
}

namespace memory_resource {

using size_type = unsigned int;

struct MemoryStats {
  size_type real_memory_used{0}; // currently used and dirty script memory
  size_type memory_used{0}; // currently used script memory

  size_type max_real_memory_used{0}; // maxumum used and dirty script memory
  size_type max_memory_used{0}; // maxumum used script memory

  size_type memory_limit{0}; // size of script memory arena
};

template<typename Allocator>
void *universal_reallocate(Allocator &alloc, void *mem, size_type new_size, size_type old_size) {
  if (void *new_mem = alloc.try_expand(mem, new_size, old_size)) {
    memory_debug("reallocate %d to %d, reallocated address %p\n", old_size, new_size, new_mem);
    return new_mem;
  }

  void *new_mem = alloc.allocate(new_size);
  if (new_mem != nullptr) {
    memcpy(new_mem, mem, old_size);
    alloc.deallocate(mem, old_size);
  }
  return new_mem;
}
} // namespace memory_resource