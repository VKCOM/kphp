#pragma once
#include <cstdarg>
#include <cstdio>
#include <cstring>

// #define DEBUG_MEMORY

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

class MemoryStats {
public:
  size_type real_memory_used{0}; // currently used and dirty memory
  size_type memory_used{0}; // currently used memory

  size_type max_real_memory_used{0}; // maxumum used and dirty memory
  size_type max_memory_used{0}; // maxumum used memory

  size_type memory_limit{0}; // size of memory arena

  size_type reserved{0}; // reserved memory
};

} // namespace memory_resource