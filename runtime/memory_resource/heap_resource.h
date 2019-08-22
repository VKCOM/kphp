#pragma once

#include <memory>

#include "common/wrappers/likely.h"

#include "runtime/memory_resource/memory_resource.h"
#include "runtime/php_assert.h"

namespace memory_resource {

class heap_resource {
public:
  void *allocate(size_type size) {
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

  void *allocate0(size_type size) {
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

  void *reallocate(void *mem, size_type new_size, size_type old_size) {
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

  void deallocate(void *mem, size_type size) {
    memory_used_ -= size;

    std::free(mem);
    memory_debug("heap deallocate %u at %p\n", size, mem);
  }

  size_type memory_used() const { return memory_used_; }

private:
  size_type memory_used_{0};
};

} // namespace memory_resource
