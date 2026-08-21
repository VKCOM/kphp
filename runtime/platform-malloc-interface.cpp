// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstddef>

#include "common/wrappers/likely.h"
#include "runtime-common/core/allocator/platform-malloc-interface.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime/allocator.h"

namespace kphp::memory::platform {

auto alloc(size_t size) noexcept -> void* {
  if (unlikely(size > MALLOC_REPLACER_MAX_ALLOC - MALLOC_REPLACER_SIZE_OFFSET)) {
    php_warning("attempt to allocate too much memory by malloc replacer : %lu", size);
    return nullptr;
  }

  const size_t real_size{size + MALLOC_REPLACER_SIZE_OFFSET};
  void* ptr{dl::heap_allocate(real_size)};

  if (unlikely(ptr == nullptr)) {
    php_warning("not enough platform memory to allocate: %lu", size);
    return ptr;
  }

  *static_cast<size_t*>(ptr) = real_size;

  return static_cast<std::byte*>(ptr) + MALLOC_REPLACER_SIZE_OFFSET;
}

void free(void* ptr) noexcept {
  if (likely(ptr != nullptr)) {
    void* real_ptr{static_cast<std::byte*>(ptr) - MALLOC_REPLACER_SIZE_OFFSET};
    dl::heap_deallocate(ptr, reinterpret_cast<size_t>(real_ptr));
  }
}

} // namespace kphp::memory::platform
