#include "runtime/memory_resource/heap_resource.h"

#include <csignal>
#include <cstdlib>

#include "common/wrappers/likely.h"

#include "runtime/critical_section.h"
#include "runtime/php_assert.h"

namespace memory_resource {

void *heap_resource::allocate(size_type size) noexcept {
  dl::CriticalSectionGuard lock;
  void *mem = std::malloc(size);
  if (unlikely(!mem)) {
    php_warning("Can't heap_allocate %u bytes", size);
    raise(SIGUSR2);
    return nullptr;
  }

  memory_debug("heap allocate %u at %p\n", size, mem);
  memory_used_ += size;
  return mem;
}

void *heap_resource::allocate0(size_type size) noexcept {
  dl::CriticalSectionGuard lock;
  void *mem = std::calloc(1, size);
  if (unlikely(!mem)) {
    php_warning("Can't heap_allocate0 %u bytes", size);
    raise(SIGUSR2);
    return nullptr;
  }

  memory_debug("heap allocate0 %u at %p\n", size, mem);
  memory_used_ += size;
  return mem;
}

void *heap_resource::reallocate(void *mem, size_type new_size, size_type old_size) noexcept {
  dl::CriticalSectionGuard lock;
  mem = std::realloc(mem, new_size);
  memory_debug("heap reallocate %u at %p\n", old_size, mem);
  if (unlikely(!mem)) {
    php_warning("Can't heap_reallocate from %u to %u bytes", old_size, new_size);
    raise(SIGUSR2);
    return nullptr;
  }
  memory_used_ += (new_size - old_size);
  return mem;
}

void heap_resource::deallocate(void *mem, size_type size) noexcept {
  dl::CriticalSectionGuard lock;
  memory_used_ -= size;

  std::free(mem);
  memory_debug("heap deallocate %u at %p\n", size, mem);
}

} // namespace memory_resource
