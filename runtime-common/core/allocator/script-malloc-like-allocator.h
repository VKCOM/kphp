// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "common/wrappers/likely.h"
#include "runtime-common/core/allocator/runtime-allocator.h"
#include "runtime-common/core/utils/kphp-assert-core.h"

namespace kphp {

namespace memory {

namespace script {

constexpr int64_t MALLOC_REPLACER_SIZE_OFFSET = sizeof(size_t);
constexpr uint64_t MALLOC_REPLACER_MAX_ALLOC = 0xFFFFFF00;

inline void *alloc(size_t size) noexcept {
  if (unlikely(size > MALLOC_REPLACER_MAX_ALLOC - MALLOC_REPLACER_SIZE_OFFSET)) {
    php_warning("attempt to allocate too much memory by malloc replacer : %lu", size);
    return nullptr;
  }
  const size_t real_size{size + MALLOC_REPLACER_SIZE_OFFSET};
  void *ptr{RuntimeAllocator::get().alloc_script_memory(real_size)};

  if (unlikely(ptr == nullptr)) {
    php_warning("not enough script memory to allocate: %lu", size);
    return ptr;
  }
  *static_cast<size_t *>(ptr) = real_size;
  return static_cast<std::byte *>(ptr) + MALLOC_REPLACER_SIZE_OFFSET;
}

inline void *calloc(size_t num, size_t size) noexcept {
  void *ptr{kphp::memory::script::alloc(num * size)};
  if (unlikely(ptr == nullptr)) {
    return nullptr;
  }
  return std::memset(ptr, 0, num * size);
}

inline void free(void *ptr) noexcept {
  if (likely(ptr != nullptr)) {
    void *real_ptr{static_cast<std::byte *>(ptr) - MALLOC_REPLACER_SIZE_OFFSET};
    RuntimeAllocator::get().free_script_memory(real_ptr, *static_cast<size_t *>(real_ptr));
  }
}

inline void *realloc(void *ptr, size_t new_size) noexcept {
  if (unlikely(ptr == nullptr)) {
    return alloc(new_size);
  }

  if (unlikely(new_size == 0)) {
    kphp::memory::script::free(ptr);
    return nullptr;
  }

  void *real_ptr{static_cast<std::byte *>(ptr) - sizeof(size_t)};
  const size_t old_size{*static_cast<size_t *>(real_ptr)};

  void *new_ptr{alloc(new_size)};
  if (likely(new_ptr != nullptr)) {
    std::memcpy(new_ptr, ptr, std::min(new_size, old_size));
    RuntimeAllocator::get().free_script_memory(real_ptr, old_size);
  }
  return new_ptr;
}



} // namespace script

} // namespace memory

} // namespace kphp
