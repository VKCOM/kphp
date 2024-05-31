// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstring>

#include "runtime-core/memory-resource/memory_resource.h"

namespace memory_resource {
namespace details {

template<typename Allocator>
void *universal_reallocate(Allocator &alloc, void *mem, size_t new_size, size_t old_size) {
  if (void *new_mem = alloc.try_expand(mem, new_size, old_size)) {
    memory_debug("reallocate %zu to %zu, reallocated address %p\n", old_size, new_size, new_mem);
    return new_mem;
  }

  void *new_mem = alloc.allocate(new_size);
  if (new_mem != nullptr) {
    memcpy(new_mem, mem, old_size);
    alloc.deallocate(mem, old_size);
  }
  return new_mem;
}

} // namespace details
} // namespace memory_resource
