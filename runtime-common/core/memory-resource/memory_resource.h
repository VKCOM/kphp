// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <cinttypes>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <limits>
#include <map>

// #define DEBUG_MEMORY
#include <sys/mman.h>
extern char *malloc_tracing_buffer;
extern char *dummy_allocator_current_ptr;
extern uint64_t dummy_allocator_mem_usage;

template<class T>
struct DummyAllocator
{
  using value_type = T;

  DummyAllocator() = default;

  template<class U>
  constexpr DummyAllocator(const DummyAllocator <U>&) noexcept {}

  [[nodiscard]] T* allocate(std::size_t n) {
    if (dummy_allocator_mem_usage >= (1800 * (1 << 20))) {
      malloc_tracing_buffer = static_cast<char *>(mmap(nullptr, (2000 * (1 << 20)) , PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0));
      dummy_allocator_current_ptr = malloc_tracing_buffer;
      dummy_allocator_mem_usage = 0;
    }
    auto *cur = dummy_allocator_current_ptr;
    dummy_allocator_current_ptr += (n * sizeof(T)) + 1;
    dummy_allocator_mem_usage += n * sizeof(T);

    return reinterpret_cast<T*>(cur);
  }

  void deallocate([[maybe_unused]] T* p, [[maybe_unused]] std::size_t n) noexcept {}
};

template<class T, class U>
bool operator==(const DummyAllocator <T>&, const DummyAllocator <U>&) { return true; }

template<class T, class U>
bool operator!=(const DummyAllocator <T>&, const DummyAllocator <U>&) { return false; }

using malloc_tracing_key_t = uint64_t;
using malloc_tracing_val_t = std::array<void*, 16>; // uint64_t;//
using dummy_allocator_t = DummyAllocator<std::pair<malloc_tracing_key_t const, malloc_tracing_val_t>>;
using malloc_tracing_storage_t = std::map<malloc_tracing_key_t, malloc_tracing_val_t, std::less<malloc_tracing_key_t>, dummy_allocator_t>;

extern malloc_tracing_storage_t* const malloc_tracing_storage;

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

constexpr size_t memory_buffer_limit() noexcept {
  return std::numeric_limits<uint32_t>::max();
}

class MemoryStats {
public:
  size_t real_memory_used{0}; // currently used and dirty memory
  size_t memory_used{0}; // currently used memory

  size_t max_real_memory_used{0}; // maximum used and dirty memory
  size_t max_memory_used{0}; // maximum used memory

  size_t memory_limit{0}; // size of memory arena

  size_t defragmentation_calls{0}; // the number of defragmentation process calls

  size_t huge_memory_pieces{0}; // the number of huge memory pirces (in rb tree)
  size_t small_memory_pieces{0}; // the number of small memory pieces (in lists)

  size_t total_allocations{0}; // the total number of allocations
  size_t total_memory_allocated{0}; // the total amount of the memory allocated (doesn't take the freed memory into the account)
};

} // namespace memory_resource
