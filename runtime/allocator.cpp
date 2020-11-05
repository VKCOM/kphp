// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/allocator.h"

#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <malloc.h>
#include <unistd.h>

#include "common/algorithms/find.h"
#include "common/containers/final_action.h"
#include "common/wrappers/likely.h"

#include "runtime/critical_section.h"
#include "runtime/memory_resource/dealer.h"
#include "runtime/php_assert.h"

namespace dl {
namespace {

memory_resource::Dealer &get_memory_dealer() noexcept {
  static memory_resource::Dealer dealer;
  return dealer;
}

volatile bool script_allocator_enabled = false;

} // namespace

long long query_num = 0;

void set_current_script_allocator(memory_resource::unsynchronized_pool_resource &resource, bool force_enable) noexcept {
  get_memory_dealer().set_current_script_resource(resource);
  if (force_enable) {
    php_assert(!script_allocator_enabled);
    script_allocator_enabled = true;
  }
}

void restore_default_script_allocator(bool force_disable) noexcept {
  get_memory_dealer().restore_default_script_resource();
  if (force_disable) {
    php_assert(script_allocator_enabled);
    script_allocator_enabled = false;
  }
}

const memory_resource::MemoryStats &get_script_memory_stats() noexcept {
  return get_memory_dealer().current_script_resource().get_memory_stats();
}

size_t get_heap_memory_used() noexcept {
  return get_memory_dealer().get_heap_resource().memory_used();
}

void global_init_script_allocator() noexcept {
  auto &dealer = get_memory_dealer();
  php_assert(dealer.heap_script_resource_replacer());
  php_assert(!is_malloc_replaced());
  php_assert(!script_allocator_enabled);
  php_assert(!query_num);

  CriticalSectionGuard lock;
  dealer.drop_replacer();
  query_num++;
}

void init_script_allocator(void *buffer, size_t buffer_size) noexcept {
  auto &dealer = get_memory_dealer();
  php_assert(!dealer.heap_script_resource_replacer());
  php_assert(dealer.is_default_allocator_used());
  php_assert(!is_malloc_replaced());

  CriticalSectionGuard lock;
  dealer.current_script_resource().init(buffer, buffer_size);
  script_allocator_enabled = true;
  query_num++;
}

void free_script_allocator() noexcept {
  auto &dealer = get_memory_dealer();
  php_assert(!dealer.heap_script_resource_replacer());
  php_assert(dealer.is_default_allocator_used());
  php_assert(!is_malloc_replaced());

  script_allocator_enabled = false;
}

void *allocate(size_t size) noexcept {
  php_assert(size);
  auto &dealer = get_memory_dealer();
  if (auto heap_replacer = dealer.heap_script_resource_replacer()) {
    return heap_replacer->allocate(size);
  }
  if (unlikely(!script_allocator_enabled)) {
    php_critical_error("Trying to call allocate for non runned script, n = %zu", size);
    return nullptr;
  }

  return dealer.current_script_resource().allocate(size);
}

void *allocate0(size_t size) noexcept {
  php_assert(size);
  auto &dealer = get_memory_dealer();
  if (auto heap_replacer = dealer.heap_script_resource_replacer()) {
    return heap_replacer->allocate0(size);
  }
  if (unlikely(!script_allocator_enabled)) {
    php_critical_error("Trying to call allocate0 for non runned script, n = %zu", size);
    return nullptr;
  }

  return dealer.current_script_resource().allocate0(size);
}

void *reallocate(void *mem, size_t new_size, size_t old_size) noexcept {
  php_assert(new_size > old_size);
  auto &dealer = get_memory_dealer();
  if (auto heap_replacer = dealer.heap_script_resource_replacer()) {
    return heap_replacer->reallocate(mem, new_size, old_size);
  }
  if (unlikely(!script_allocator_enabled)) {
    php_critical_error("Trying to call reallocate for non runned script, p = %p, new_size = %zu, old_size = %zu", mem, new_size, old_size);
    return mem;
  }

  return dealer.current_script_resource().reallocate(mem, new_size, old_size);
}

void deallocate(void *mem, size_t size) noexcept {
  php_assert(size);
  auto &dealer = get_memory_dealer();
  if (auto heap_replacer = dealer.heap_script_resource_replacer()) {
    return heap_replacer->deallocate(mem, size);
  }

  if (script_allocator_enabled) {
    dealer.current_script_resource().deallocate(mem, size);
  }
}

void *heap_allocate(size_t size) noexcept {
  php_assert(!query_num || !is_malloc_replaced());
  return get_memory_dealer().get_heap_resource().allocate(size);
}

void *heap_reallocate(void *mem, size_t new_size, size_t old_size) noexcept {
  return get_memory_dealer().get_heap_resource().reallocate(mem, new_size, old_size);
}

void heap_deallocate(void *mem, size_t size) noexcept {
  return get_memory_dealer().get_heap_resource().deallocate(mem, size);
}

namespace {

// guaranteed alignment of dl::allocate
constexpr size_t MALLOC_REPLACER_SIZE_OFFSET = 8;
constexpr size_t MALLOC_REPLACER_MAX_ALLOC = 0xFFFFFF00;

} // namespace

void *script_allocator_malloc(size_t size) noexcept {
  static_assert(sizeof(size_t) <= MALLOC_REPLACER_SIZE_OFFSET, "small size offset");
  if (size > MALLOC_REPLACER_MAX_ALLOC - MALLOC_REPLACER_SIZE_OFFSET) {
    php_warning("attempt to allocate too much memory: %lu", size);
    return nullptr;
  }
  const size_t real_allocate = size + MALLOC_REPLACER_SIZE_OFFSET;
  void *mem = allocate(real_allocate);

  if (unlikely(!mem)) {
    php_warning("not enough memory to continue: %lu", size);
    return mem;
  }
  *static_cast<size_t *>(mem) = real_allocate;
  return static_cast<char *>(mem) + MALLOC_REPLACER_SIZE_OFFSET;
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

void script_allocator_free(void *mem) noexcept {
  if (mem) {
    mem = static_cast<char *>(mem) - MALLOC_REPLACER_SIZE_OFFSET;
    deallocate(mem, *static_cast<size_t *>(mem));
  }
}

namespace {

class MallocHooksSwitcher : vk::not_copyable {
public:
  static MallocHooksSwitcher &get() noexcept {
    static MallocHooksSwitcher switcher;
    return switcher;
  }

  void switch_hooks(bool malloc_hooks_are_replaced_before) noexcept {
    php_assert(malloc_hooks_are_replaced_before == malloc_hooks_are_replaced_);
    CriticalSectionGuard critical_section;
    std::swap(malloc_hook_, __malloc_hook);
    std::swap(realloc_hook_, __realloc_hook);
    std::swap(memalign_hook_, __memalign_hook);
    std::swap(free_hook_, __free_hook);
    malloc_hooks_are_replaced_ = !malloc_hooks_are_replaced_;
  }

  bool are_malloc_hooks_replaced() const noexcept {
    return malloc_hooks_are_replaced_;
  }

private:
  MallocHooksSwitcher() :
    malloc_hook_{[](size_t size, const void *) {
      return script_allocator_malloc(size);
    }},
    realloc_hook_{[](void *ptr, size_t size, const void *) {
      return script_allocator_realloc(ptr, size);
    }},
    memalign_hook_{[](size_t alignment, size_t size, const void *) {
      // script allocator gives addresses aligned to 8 bytes
      php_assert(alignment <= 8);
      return script_allocator_malloc(size);
    }},
    free_hook_{[](void *ptr, const void *) {
      return script_allocator_free(ptr);
    }} {}

  decltype(__malloc_hook) malloc_hook_{nullptr};
  decltype(__realloc_hook) realloc_hook_{nullptr};
  decltype(__memalign_hook) memalign_hook_{nullptr};
  decltype(__free_hook) free_hook_{nullptr};
  bool malloc_hooks_are_replaced_{false};
};

} // namespace

bool is_malloc_replaced() noexcept {
  return MallocHooksSwitcher::get().are_malloc_hooks_replaced();
}

void replace_malloc_with_script_allocator() noexcept {
  MallocHooksSwitcher::get().switch_hooks(false);
}

void rollback_malloc_replacement() noexcept {
  MallocHooksSwitcher::get().switch_hooks(true);
}

} // namespace dl

// replace global operators new and delete for linked C++ code
void *operator new(size_t size) {
  auto res = std::malloc(size);
  if (!res) {
    php_critical_error("nullptr from malloc");
  }
  return res;
}

void *operator new(size_t size, const std::nothrow_t &) noexcept {
  return std::malloc(size);
}

void *operator new[](size_t size) {
  return operator new(size);
}

void *operator new[](size_t size, const std::nothrow_t &) noexcept {
  return operator new(size, std::nothrow);
}

void operator delete(void *mem) noexcept {
  return std::free(mem);
}

void operator delete(void *mem, const std::nothrow_t &) noexcept {
  return std::free(mem);
}

void operator delete[](void *mem) noexcept {
  return std::free(mem);
}

void operator delete[](void *mem, const std::nothrow_t &) noexcept {
  return std::free(mem);
}

void operator delete(void *mem, size_t) noexcept {
  return std::free(mem);
}

void operator delete[](void *mem, size_t) noexcept {
  return std::free(mem);
}
