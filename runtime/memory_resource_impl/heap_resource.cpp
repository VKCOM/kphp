// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/memory_resource_impl//heap_resource.h"

#include <csignal>
#include <cstdlib>

#include "common/wrappers/likely.h"

#include "runtime/critical_section.h"
#include "runtime/php_assert.h"

namespace memory_resource {

void *heap_resource::allocate(size_t size) noexcept {
  dl::CriticalSectionGuard lock;
  void *mem = std::malloc(size);
  if (unlikely(!mem)) {
    php_out_of_memory_warning("Can't heap_allocate %zu bytes", size);
    raise(SIGUSR2);
    return nullptr;
  }

  memory_debug("heap allocate %zu at %p\n", size, mem);
  memory_used_ += size;
  return mem;
}

void *heap_resource::allocate0(size_t size) noexcept {
  dl::CriticalSectionGuard lock;
  void *mem = std::calloc(1, size);
  if (unlikely(!mem)) {
    php_out_of_memory_warning("Can't heap_allocate0 %zu bytes", size);
    raise(SIGUSR2);
    return nullptr;
  }

  memory_debug("heap allocate0 %zu at %p\n", size, mem);
  memory_used_ += size;
  return mem;
}

void *heap_resource::reallocate(void *mem, size_t new_size, size_t old_size) noexcept {
  dl::CriticalSectionGuard lock;
  mem = std::realloc(mem, new_size);
  memory_debug("heap reallocate %zu at %p\n", old_size, mem);
  if (unlikely(!mem)) {
    php_out_of_memory_warning("Can't heap_reallocate from %zu to %zu bytes", old_size, new_size);
    raise(SIGUSR2);
    return nullptr;
  }
  memory_used_ += (new_size - old_size);
  return mem;
}

void heap_resource::deallocate(void *mem, size_t size) noexcept {
  dl::CriticalSectionGuard lock;
  memory_used_ -= size;

  std::free(mem);
  memory_debug("heap deallocate %zu at %p\n", size, mem);
}

} // namespace memory_resource
