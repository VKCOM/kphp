// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/allocator.h"

#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <unistd.h>

#include "common/algorithms/find.h"
#include "common/containers/final_action.h"
#include "common/fast-backtrace.h"
#include "common/macos-ports.h"
#include "common/wrappers/likely.h"

#include "runtime/critical_section.h"
#include "runtime/kphp-backtrace.h"
#include "runtime/memory_resource_impl//dealer.h"
#include "runtime/php_assert.h"
#include "server/server-log.h"

namespace dl {
namespace {

memory_resource::Dealer& get_memory_dealer() noexcept {
  static memory_resource::Dealer dealer;
  return dealer;
}

} // namespace

bool script_allocator_enabled = false;
long long query_num = 0;

memory_resource::unsynchronized_pool_resource& get_default_script_allocator() noexcept {
  php_assert(script_allocator_enabled);
  php_assert(get_memory_dealer().is_default_allocator_used());
  return get_memory_dealer().current_script_resource();
}

void set_current_script_allocator(memory_resource::unsynchronized_pool_resource& resource, bool force_enable) noexcept {
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

const memory_resource::MemoryStats& get_script_memory_stats() noexcept {
  return get_memory_dealer().current_script_resource().get_memory_stats();
}

size_t get_heap_memory_used() noexcept {
  return get_memory_dealer().get_heap_resource().memory_used();
}

void global_init_script_allocator() noexcept {
  auto& dealer = get_memory_dealer();
  php_assert(dealer.heap_script_resource_replacer());
  php_assert(!is_malloc_replaced());
  php_assert(!script_allocator_enabled);
  php_assert(!query_num);

  CriticalSectionGuard lock;
  dealer.drop_replacer();
  query_num++;
}

void init_script_allocator(void* buffer, size_t script_mem_size, size_t oom_handling_mem_size) noexcept {
  auto& dealer = get_memory_dealer();
  php_assert(!dealer.heap_script_resource_replacer());
  php_assert(dealer.is_default_allocator_used());
  php_assert(!is_malloc_replaced());

  CriticalSectionGuard lock;
  dealer.current_script_resource().init(buffer, script_mem_size, oom_handling_mem_size);
  script_allocator_enabled = true;
  query_num++;
}

void free_script_allocator() noexcept {
  auto& dealer = get_memory_dealer();
  php_assert(!dealer.heap_script_resource_replacer());
  php_assert(dealer.is_default_allocator_used());
  php_assert(!is_malloc_replaced());

  script_allocator_enabled = false;
}

void* allocate(size_t size) noexcept {
  php_assert(size);
  auto& dealer = get_memory_dealer();
  if (auto* heap_replacer = dealer.heap_script_resource_replacer()) {
    return heap_replacer->allocate(size);
  }
  if (unlikely(!script_allocator_enabled)) {
    php_critical_error("Trying to call allocate for non runned script, n = %zu", size);
    return nullptr;
  }

  return dealer.current_script_resource().allocate(size);
}

void* allocate0(size_t size) noexcept {
  php_assert(size);
  auto& dealer = get_memory_dealer();
  if (auto* heap_replacer = dealer.heap_script_resource_replacer()) {
    return heap_replacer->allocate0(size);
  }
  if (unlikely(!script_allocator_enabled)) {
    php_critical_error("Trying to call allocate0 for non runned script, n = %zu", size);
    return nullptr;
  }

  return dealer.current_script_resource().allocate0(size);
}

void* reallocate(void* mem, size_t new_size, size_t old_size) noexcept {
  php_assert(new_size > old_size);
  auto& dealer = get_memory_dealer();
  if (auto* heap_replacer = dealer.heap_script_resource_replacer()) {
    return heap_replacer->reallocate(mem, new_size, old_size);
  }
  if (unlikely(!script_allocator_enabled)) {
    php_critical_error("Trying to call reallocate for non runned script, p = %p, new_size = %zu, old_size = %zu", mem, new_size, old_size);
    return mem;
  }

  return dealer.current_script_resource().reallocate(mem, new_size, old_size);
}

void deallocate(void* mem, size_t size) noexcept {
  php_assert(size);
  auto& dealer = get_memory_dealer();
  if (auto* heap_replacer = dealer.heap_script_resource_replacer()) {
    return heap_replacer->deallocate(mem, size);
  }

  if (script_allocator_enabled) {
    dealer.current_script_resource().deallocate(mem, size);
  }
}

void* heap_allocate(size_t size) noexcept {
  php_assert(!query_num || !is_malloc_replaced());
  return get_memory_dealer().get_heap_resource().allocate(size);
}

void* heap_reallocate(void* mem, size_t new_size, size_t old_size) noexcept {
  return get_memory_dealer().get_heap_resource().reallocate(mem, new_size, old_size);
}

void heap_deallocate(void* mem, size_t size) noexcept {
  return get_memory_dealer().get_heap_resource().deallocate(mem, size);
}

namespace {

class MallocStateHolder : vk::not_copyable {
public:
  static MallocStateHolder& get() noexcept {
    static MallocStateHolder state;
    return state;
  }

  void replace_malloc(bool is_malloc_replaced_before) noexcept {
    php_assert(is_malloc_replaced_before == is_malloc_replaced_);

    if (!is_malloc_replaced_before) {
      last_malloc_replacement_backtrace_size_ = fast_backtrace(last_malloc_replacement_backtrace_.data(), last_malloc_replacement_backtrace_.size());
    }

    is_malloc_replaced_ = !is_malloc_replaced_;
  }

  bool is_malloc_replaced() const noexcept {
    return is_malloc_replaced_;
  }

  std::pair<void* const*, int> get_last_malloc_replacement_backtrace() const noexcept {
    return {last_malloc_replacement_backtrace_.data(), last_malloc_replacement_backtrace_size_};
  }

private:
  MallocStateHolder() = default;
  bool is_malloc_replaced_{false};
  std::array<void*, 128> last_malloc_replacement_backtrace_{};
  int last_malloc_replacement_backtrace_size_{0};
};

} // namespace

bool is_malloc_replaced() noexcept {
  return MallocStateHolder::get().is_malloc_replaced();
}

void replace_malloc_with_script_allocator() noexcept {
  MallocStateHolder::get().replace_malloc(false);
}

void rollback_malloc_replacement() noexcept {
  MallocStateHolder::get().replace_malloc(true);
}

void write_last_malloc_replacement_stacktrace(char* buf, size_t buf_size) noexcept {
  auto malloc_replacement_rollback = temporary_rollback_malloc_replacement();
  auto [raw_backtrace, backtrace_size] = MallocStateHolder::get().get_last_malloc_replacement_backtrace();
  parse_kphp_backtrace(buf, buf_size, raw_backtrace, backtrace_size);
}

MemoryReplacementGuard::MemoryReplacementGuard(memory_resource::unsynchronized_pool_resource& memory_resource, bool force_enable_disable)
    : force_enable_disable_(force_enable_disable) {
  dl::enter_critical_section();
  dl::set_current_script_allocator(memory_resource, force_enable_disable_);
}

MemoryReplacementGuard::~MemoryReplacementGuard() {
  dl::restore_default_script_allocator(force_enable_disable_);
  dl::leave_critical_section();
}

} // namespace dl

// sanitizers aren't happy with custom realization of malloc-like functions
#if !ASAN_ENABLED and !defined(__APPLE__)

extern "C" void* __libc_malloc(size_t size);
extern "C" void* __libc_calloc(size_t nmemb, size_t size);
extern "C" void* __libc_realloc(void* mem, size_t new_size);
extern "C" void __libc_free(void* mem);
extern "C" void* __libc_memalign(size_t alignment, size_t size);

void* malloc(size_t size) {
  return dl::is_malloc_replaced() ? dl::script_allocator_malloc(size) : __libc_malloc(size);
}

void* calloc(size_t nmemb, size_t size) {
  return dl::is_malloc_replaced() ? dl::script_allocator_calloc(nmemb, size) : __libc_calloc(nmemb, size);
}

void* realloc(void* mem, size_t new_size) {
  return dl::is_malloc_replaced() ? dl::script_allocator_realloc(mem, new_size) : __libc_realloc(mem, new_size);
}

void free(void* mem) {
  return dl::is_malloc_replaced() ? dl::script_allocator_free(mem) : __libc_free(mem);
}

void* aligned_alloc(size_t alignment, size_t size) {
  if (dl::is_malloc_replaced()) {
    // script allocator gives addresses aligned to 8 bytes
    php_assert(alignment <= 8);
    return dl::script_allocator_malloc(size);
  }
  return __libc_memalign(alignment, size);
}

void* memalign(size_t alignment, size_t size) {
  return aligned_alloc(alignment, size);
}

// put realization posix_memalign function here if you want to use it under script allocator

#endif

// replace global operators new and delete for linked C++ code
void* operator new(size_t size) {
  auto* res = std::malloc(size);
  if (!res) {
    php_critical_error("nullptr from malloc");
  }
  return res;
}

void* operator new(size_t size, const std::nothrow_t&) noexcept {
  return std::malloc(size);
}

void* operator new[](size_t size) {
  return operator new(size);
}

void* operator new[](size_t size, const std::nothrow_t&) noexcept {
  return operator new(size, std::nothrow);
}

void operator delete(void* mem) noexcept {
  return std::free(mem);
}

void operator delete(void* mem, const std::nothrow_t&) noexcept {
  return std::free(mem);
}

void operator delete[](void* mem) noexcept {
  return std::free(mem);
}

void operator delete[](void* mem, const std::nothrow_t&) noexcept {
  return std::free(mem);
}

void operator delete(void* mem, size_t) noexcept {
  return std::free(mem);
}

void operator delete[](void* mem, size_t) noexcept {
  return std::free(mem);
}
