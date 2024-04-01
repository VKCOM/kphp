// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <cinttypes>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <limits>

#include "common/stats/provider.h"

// #define DEBUG_MEMORY

inline void memory_debug(const char *format, ...) __attribute__((format(printf, 1, 2)));

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

constexpr size_t memory_buffer_limit() noexcept {
  return std::numeric_limits<uint32_t>::max();
}

class MemoryStats {
public:
  size_t real_memory_used{0}; // currently used and dirty memory
  size_t memory_used{0};      // currently used memory

  size_t max_real_memory_used{0}; // maximum used and dirty memory
  size_t max_memory_used{0};      // maximum used memory

  size_t memory_limit{0}; // size of memory arena

  size_t defragmentation_calls{0}; // the number of defragmentation process calls

  size_t huge_memory_pieces{0};  // the number of huge memory pirces (in rb tree)
  size_t small_memory_pieces{0}; // the number of small memory pieces (in lists)

  size_t total_allocations{0};      // the total number of allocations
  size_t total_memory_allocated{0}; // the total amount of the memory allocated (doesn't take the freed memory into the account)

  void write_stats_to(stats_t *stats, const char *prefix) const noexcept;
};

} // namespace memory_resource
