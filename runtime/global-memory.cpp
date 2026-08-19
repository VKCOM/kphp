//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2026 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstring>

#include "runtime-common/core/allocator/global-memory.h"
#include "runtime/allocator.h"

namespace kphp::memory::global {

auto alloc(size_t size) noexcept -> void* {
  return dl::heap_allocate(size);
}

auto alloc0(size_t size) noexcept -> void* {
  void* ptr = dl::heap_allocate(size);
  if (ptr != nullptr) {
    memset(ptr, 0, size);
  }
  return ptr;
}

auto realloc(void* mem, size_t new_size, size_t old_size) noexcept -> void* {
  return dl::heap_reallocate(mem, new_size, old_size);
}

auto free(void* mem, size_t size) noexcept -> void {
  dl::heap_deallocate(mem, size);
}

} // namespace kphp::memory::global
