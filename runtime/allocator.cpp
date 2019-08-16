#include <climits>
#include <cstdlib>
#include <cstring>
#include <signal.h>
#include <unistd.h>

#include "common/wrappers/likely.h"

#include "runtime/allocator.h"
#include "runtime/memory_resource/unsynchronized_pool_resource.h"
#include "runtime/php_assert.h"

void check_stack_overflow() __attribute__ ((weak));

void check_stack_overflow() {
  //pass;
}

namespace dl {
// feel free to replace with monotonic_buffer_resource for tests or debugging
using ScriptMemoryResource = memory_resource::unsynchronized_pool_resource;
inline ScriptMemoryResource &get_script_mem_resource() {
  static ScriptMemoryResource resource;
  return resource;
}

volatile int in_critical_section = 0;
volatile long long pending_signals = 0;

long long query_num = 0;
volatile bool script_runned = false;
volatile bool replace_malloc_with_script_allocator = false;
volatile bool replace_script_allocator_with_malloc = false;
volatile bool forbid_malloc_allocations = false;

static bool script_allocator_inited = false;

size_type static_memory_used = 0;

void enter_critical_section() {
  check_stack_overflow();
  php_assert (in_critical_section >= 0);
  in_critical_section++;
}

void leave_critical_section() {
  in_critical_section--;
  php_assert (in_critical_section >= 0);
  if (pending_signals && in_critical_section <= 0) {
    for (int i = 0; i < (int)sizeof(pending_signals) * 8; i++) {
      if ((pending_signals >> i) & 1) {
        raise(i);
      }
    }
  }
}

struct CriticalSectionGuard {
  CriticalSectionGuard() { dl::enter_critical_section(); }
  ~CriticalSectionGuard() { dl::leave_critical_section(); }
};

const memory_resource::MemoryStats &get_script_memory_stats() {
  return get_script_mem_resource().get_memory_stats();
}

void global_init_script_allocator() {
  CriticalSectionGuard lock;
  query_num++;
}

void init_script_allocator(void *buffer, size_type buffer_size) {
  in_critical_section = 0;
  pending_signals = 0;

  CriticalSectionGuard lock;

  script_allocator_inited = true;
  replace_malloc_with_script_allocator = false;
  replace_script_allocator_with_malloc = false;
  forbid_malloc_allocations = false;

  get_script_mem_resource().init(buffer, buffer_size);

  query_num++;
  script_runned = true;
}

void free_script_allocator() {
  CriticalSectionGuard lock;

  script_runned = false;
  php_assert (!forbid_malloc_allocations);
  php_assert (!replace_script_allocator_with_malloc);
  php_assert (!replace_malloc_with_script_allocator);
}

void *allocate(size_type size) {
  if (!script_allocator_inited || replace_script_allocator_with_malloc) {
    return heap_allocate(size);
  }

  if (unlikely(!script_runned)) {
    php_critical_error("Trying to call allocate for non runned script, n = %u", size);
    return nullptr;
  }

  CriticalSectionGuard lock;
  php_assert (size);
  return get_script_mem_resource().allocate(size);
}

void *allocate0(size_type size) {
  if (!script_allocator_inited || replace_script_allocator_with_malloc) {
    return heap_allocate0(size);
  }

  if (unlikely(!script_runned)) {
    php_critical_error("Trying to call allocate0 for non runned script, n = %u", size);
    return nullptr;
  }

  return memset(allocate(size), 0, size);
}

void *reallocate(void *mem, size_type new_size, size_type old_size) {
  if (!script_allocator_inited || replace_script_allocator_with_malloc) {
    heap_reallocate(&mem, new_size, &old_size);
    return mem;
  }

  if (unlikely(!script_runned)) {
    php_critical_error("Trying to call reallocate for non runned script, p = %p, new_size = %u, old_size = %u", mem, new_size, old_size);
    return mem;
  }

  CriticalSectionGuard lock;
  php_assert (new_size > old_size);
  return get_script_mem_resource().reallocate(mem, new_size, old_size);
}

void deallocate(void *mem, size_type size) {
  if (!script_allocator_inited || replace_script_allocator_with_malloc) {
    return heap_deallocate(&mem, &size);
  }

  if (script_runned) {
    CriticalSectionGuard lock;
    php_assert (size);
    get_script_mem_resource().deallocate(mem, size);
  }
}

void *heap_allocate(size_type size) {
  php_assert (!query_num || !replace_malloc_with_script_allocator || replace_script_allocator_with_malloc);
  CriticalSectionGuard lock;
  php_assert (size);
  php_assert(!forbid_malloc_allocations);
  void *mem = malloc(size);
  if (unlikely(!mem)) {
    php_warning("Can't heap_allocate %u bytes", size);
    raise(SIGUSR2);
    return nullptr;
  }

  memory_debug("heap allocate %u at %p\n", size, mem);
  static_memory_used += size;
  return mem;
}

void *heap_allocate0(size_type size) {
  php_assert (!query_num || replace_script_allocator_with_malloc);
  CriticalSectionGuard lock;
  php_assert (size);
  php_assert(!forbid_malloc_allocations);
  void *mem = calloc(1, size);
  if (unlikely(!mem)) {
    php_warning("Can't heap_allocate0 %u bytes", size);
    raise(SIGUSR2);
    return nullptr;
  }

  memory_debug("heap allocate0 %u at %p\n", size, mem);
  static_memory_used += size;
  return mem;
}

void heap_reallocate(void **mem, size_type new_size, size_type *old_size) {
  CriticalSectionGuard lock;
  static_memory_used -= *old_size;
  php_assert(!forbid_malloc_allocations);
  void *old_mem = *mem;
  *mem = realloc(*mem, new_size);
  memory_debug("heap reallocate %u at %p\n", *old_size, *mem);
  if (unlikely(*mem == nullptr)) {
    php_warning("Can't heap_reallocate from %u to %u bytes", *old_size, new_size);
    raise(SIGUSR2);
    *mem = old_mem;
    return;
  }
  *old_size = new_size;
  static_memory_used += *old_size;
}

void heap_deallocate(void **mem, size_type *size) {
  CriticalSectionGuard lock;
  static_memory_used -= *size;

  free(*mem);
  memory_debug("heap deallocate %u at %p\n", *size, *mem);
  *mem = nullptr;
  *size = 0;
}

const size_type MAX_ALLOC = 0xFFFFFF00;
// guaranteed alignment of dl::allocate
const size_t MAX_ALIGNMENT = 8;

void *malloc_replace(size_t size) {
  php_assert (size <= MAX_ALLOC - MAX_ALIGNMENT);
  php_assert (sizeof(size_t) <= MAX_ALIGNMENT);
  size_t real_allocate = size + MAX_ALIGNMENT;
  void *mem = nullptr;
  if (replace_malloc_with_script_allocator) {
    mem = allocate(real_allocate);
  } else {
    mem = heap_allocate(real_allocate);
  }
  if (unlikely(!mem)) {
    php_critical_error ("not enough memory to continue");
  }
  *static_cast<size_t *>(mem) = real_allocate;
  return static_cast<char *>(mem) + MAX_ALIGNMENT;
}

void free_replace(void *mem) {
  if (mem == nullptr) {
    return;
  }

  mem = static_cast<char *>(mem) - MAX_ALIGNMENT;
  if (replace_malloc_with_script_allocator) {
    deallocate(mem, *static_cast<size_t *>(mem));
  } else {
    size_type size = *static_cast<size_t *>(mem);
    heap_deallocate(&mem, &size);
  }
}
} // namespace dl

//replace global operators new and delete for linked C++ code
void *operator new(std::size_t size) {
  return dl::malloc_replace(size);
}

void *operator new(std::size_t size, const std::nothrow_t &) noexcept {
  return dl::malloc_replace(size);
}

void *operator new[](std::size_t size) {
  return dl::malloc_replace(size);
}

void *operator new[](std::size_t size, const std::nothrow_t &) noexcept {
  return dl::malloc_replace(size);
}

void operator delete(void *mem) noexcept {
  return dl::free_replace(mem);
}

void operator delete(void *mem, const std::nothrow_t &) noexcept {
  return dl::free_replace(mem);
}

void operator delete[](void *mem) noexcept {
  return dl::free_replace(mem);
}

void operator delete[](void *mem, const std::nothrow_t &) noexcept {
  return dl::free_replace(mem);
}

#if __cplusplus >= 201402L
void operator delete(void *mem, size_t) noexcept {
  return dl::free_replace(mem);
}

void operator delete[](void *mem, size_t) noexcept {
  return dl::free_replace(mem);
}
#endif
