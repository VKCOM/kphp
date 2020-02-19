#include "runtime/allocator.h"

#include <climits>
#include <cstdlib>
#include <cstring>
#include <signal.h>
#include <unistd.h>

#include "common/wrappers/likely.h"

#include "runtime/critical_section.h"
#include "runtime/memory_resource/dealer.h"
#include "runtime/php_assert.h"

namespace dl {

long long query_num = 0;
volatile bool script_runned = false;
volatile bool replace_malloc_with_script_allocator = false;

memory_resource::Dealer &get_memory_dealer() noexcept {
  static memory_resource::Dealer dealer;
  return dealer;
}

void set_script_allocator_replacement(memory_resource::synchronized_pool_resource *replacer) noexcept {
  get_memory_dealer().set_script_resource_replacer(replacer);
}

void drop_script_allocator_replacement() noexcept {
  get_memory_dealer().drop_replacer();
}

const memory_resource::MemoryStats &get_script_memory_stats() noexcept {
  return get_memory_dealer().script_default_resource().get_memory_stats();
}

size_type get_heap_memory_used() noexcept {
  return get_memory_dealer().heap_resource().memory_used();
}

void global_init_script_allocator() noexcept {
  php_assert(get_memory_dealer().heap_script_resource_replacer());
  php_assert(!get_memory_dealer().synchronized_script_resource_replacer());
  php_assert(!replace_malloc_with_script_allocator);
  php_assert(!script_runned);
  php_assert(!query_num);

  CriticalSectionGuard lock;
  get_memory_dealer().drop_replacer();
  query_num++;
}

void init_script_allocator(void *buffer, size_type buffer_size) noexcept {
  php_assert(!get_memory_dealer().heap_script_resource_replacer());
  php_assert(!get_memory_dealer().synchronized_script_resource_replacer());

  CriticalSectionGuard lock;
  get_memory_dealer().script_default_resource().init(buffer, buffer_size);
  replace_malloc_with_script_allocator = false;
  script_runned = true;
  query_num++;
}

void free_script_allocator() noexcept {
  php_assert(!get_memory_dealer().heap_script_resource_replacer());
  php_assert(!get_memory_dealer().synchronized_script_resource_replacer());

  CriticalSectionGuard lock;
  script_runned = false;
}

void *allocate(size_type size) noexcept {
  php_assert (size);
  CriticalSectionGuard lock;
  if (auto heap_replacer = get_memory_dealer().heap_script_resource_replacer()) {
    return heap_replacer->allocate(size);
  }
  if (auto synchronized_replacer = get_memory_dealer().synchronized_script_resource_replacer()) {
    return synchronized_replacer->allocate(size);
  }
  if (unlikely(!script_runned)) {
    php_critical_error("Trying to call allocate for non runned script, n = %u", size);
    return nullptr;
  }

  return get_memory_dealer().script_default_resource().allocate(size);
}

void *allocate0(size_type size) noexcept {
  php_assert (size);
  CriticalSectionGuard lock;
  if (auto heap_replacer = get_memory_dealer().heap_script_resource_replacer()) {
    return heap_replacer->allocate0(size);
  }
  if (auto synchronized_replacer = get_memory_dealer().synchronized_script_resource_replacer()) {
    return synchronized_replacer->allocate0(size);
  }
  if (unlikely(!script_runned)) {
    php_critical_error("Trying to call allocate0 for non runned script, n = %u", size);
    return nullptr;
  }

  return get_memory_dealer().script_default_resource().allocate0(size);
}

void *reallocate(void *mem, size_type new_size, size_type old_size) noexcept {
  php_assert (new_size > old_size);
  CriticalSectionGuard lock;
  if (auto heap_replacer = get_memory_dealer().heap_script_resource_replacer()) {
    return heap_replacer->reallocate(mem, new_size, old_size);
  }
  if (auto synchronized_replacer = get_memory_dealer().synchronized_script_resource_replacer()) {
    return synchronized_replacer->reallocate(mem, new_size, old_size);
  }

  if (unlikely(!script_runned)) {
    php_critical_error("Trying to call reallocate for non runned script, p = %p, new_size = %u, old_size = %u", mem, new_size, old_size);
    return mem;
  }

  return get_memory_dealer().script_default_resource().reallocate(mem, new_size, old_size);
}

void deallocate(void *mem, size_type size) noexcept {
  php_assert (size);
  CriticalSectionGuard lock;
  if (auto heap_replacer = get_memory_dealer().heap_script_resource_replacer()) {
    return heap_replacer->deallocate(mem, size);
  }
  if (auto synchronized_replacer = get_memory_dealer().synchronized_script_resource_replacer()) {
    return synchronized_replacer->deallocate(mem, size);
  }

  if (script_runned) {
    get_memory_dealer().script_default_resource().deallocate(mem, size);
  }
}

void perform_script_allocator_defragmentation() noexcept {
  if (script_runned) {
    get_memory_dealer().script_default_resource().perform_defragmentation();
  }
}

void *heap_allocate(size_type size) noexcept {
  php_assert(!query_num || !replace_malloc_with_script_allocator);
  CriticalSectionGuard lock;
  return get_memory_dealer().heap_resource().allocate(size);
}

void *heap_reallocate(void *mem, size_type new_size, size_type old_size) noexcept {
  CriticalSectionGuard lock;
  return get_memory_dealer().heap_resource().reallocate(mem, new_size, old_size);
}

void heap_deallocate(void *mem, size_type size) noexcept {
  CriticalSectionGuard lock;
  return get_memory_dealer().heap_resource().deallocate(mem, size);
}

namespace {
// guaranteed alignment of dl::allocate
const size_t MAX_ALIGNMENT = 8;

template<bool always_use_script_allocator>
void *malloc_replace_impl(size_t size) noexcept {
  static_assert(sizeof(size_t) <= MAX_ALIGNMENT, "small alignment");
  const size_type MAX_ALLOC = 0xFFFFFF00;
  php_assert (size <= MAX_ALLOC - MAX_ALIGNMENT);
  const size_t real_allocate = size + MAX_ALIGNMENT;
  void *mem = nullptr;
  if (always_use_script_allocator || replace_malloc_with_script_allocator) {
    mem = allocate(real_allocate);
  } else {
    mem = heap_allocate(real_allocate);
  }
  if (unlikely(!mem)) {
    php_critical_error ("not enough memory to continue");
    return mem;
  }
  *static_cast<size_t *>(mem) = real_allocate;
  return static_cast<char *>(mem) + MAX_ALIGNMENT;
}

template<bool always_use_script_allocator>
void free_replace_impl(void *mem) noexcept {
  if (mem) {
    mem = static_cast<char *>(mem) - MAX_ALIGNMENT;
    if (always_use_script_allocator || replace_malloc_with_script_allocator) {
      deallocate(mem, *static_cast<size_t *>(mem));
    } else {
      heap_deallocate(mem, *static_cast<size_t *>(mem));
    }
  }
}
} // namespace

void *script_allocator_malloc(size_t x) noexcept {
  return malloc_replace_impl<true>(x);
}

void *script_allocator_calloc(size_t nmemb, size_t size) noexcept {
  void *res = script_allocator_malloc(nmemb * size);
  if (unlikely(res == nullptr)) {
    return nullptr;
  }
  return memset(res, 0, nmemb * size);
}

void *script_allocator_realloc(void *p, size_t new_size) noexcept {
  if (p == nullptr) {
    return script_allocator_malloc(new_size);
  }

  if (new_size == 0) {
    script_allocator_free(p);
    return nullptr;
  }

  void *real_p = static_cast<char *>(p) - sizeof(size_t);
  const size_t old_size = *static_cast<size_t *>(real_p);

  // TODO reuse reallocate method
  void *new_p = script_allocator_malloc(new_size);
  if (likely(new_p != nullptr)) {
    memcpy(new_p, p, std::min(new_size, old_size));
    dl::deallocate(real_p, old_size);
  }
  return new_p;
}

char *script_allocator_strdup(const char *str) noexcept {
  const size_t len = strlen(str) + 1;
  char *res = static_cast<char *>(script_allocator_malloc(len));
  if (unlikely(res == nullptr)) {
    return nullptr;
  }
  memcpy(res, str, len);
  return res;
}

void script_allocator_free(void *p) noexcept {
  return free_replace_impl<true>(p);
}


void *replaceable_malloc(size_t x) noexcept {
  return malloc_replace_impl<false>(x);
}

void replaceable_free(void *p) noexcept {
  return free_replace_impl<false>(p);
}

} // namespace dl

// replace global operators new and delete for linked C++ code
void *operator new(std::size_t size) {
  return dl::replaceable_malloc(size);
}

void *operator new(std::size_t size, const std::nothrow_t &) noexcept {
  return dl::replaceable_malloc(size);
}

void *operator new[](std::size_t size) {
  return dl::replaceable_malloc(size);
}

void *operator new[](std::size_t size, const std::nothrow_t &) noexcept {
  return dl::replaceable_malloc(size);
}

void operator delete(void *mem) noexcept {
  return dl::replaceable_free(mem);
}

void operator delete(void *mem, const std::nothrow_t &) noexcept {
  return dl::replaceable_free(mem);
}

void operator delete[](void *mem) noexcept {
  return dl::replaceable_free(mem);
}

void operator delete[](void *mem, const std::nothrow_t &) noexcept {
  return dl::replaceable_free(mem);
}

#if __cplusplus >= 201402L
void operator delete(void *mem, size_t) noexcept {
  return dl::replaceable_free(mem);
}

void operator delete[](void *mem, size_t) noexcept {
  return dl::replaceable_free(mem);
}
#endif
