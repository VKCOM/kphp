// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "runtime-common/core/allocator/runtime-allocator.h"
#include "runtime-common/core/utils/kphp-assert-core.h"

namespace kphp {

namespace malloc_replace {

constexpr int64_t MALLOC_REPLACER_SIZE_OFFSET = 8;
constexpr uint64_t MALLOC_REPLACER_MAX_ALLOC = 0xFFFFFF00;

inline void *alloc(size_t size) noexcept {
  static_assert(sizeof(size_t) <= MALLOC_REPLACER_SIZE_OFFSET, "small size offset");
  if (size > MALLOC_REPLACER_MAX_ALLOC - MALLOC_REPLACER_SIZE_OFFSET) {
    php_warning("attempt to allocate too much memory: %lu", size);
    return nullptr;
  }
  const size_t real_size{size + MALLOC_REPLACER_SIZE_OFFSET};
  void *mem{RuntimeAllocator::get().alloc_script_memory(real_size)};

  if (mem == nullptr) [[unlikely]] {
    php_warning("not enough memory to continue: %lu", size);
    return mem;
  }
  *static_cast<size_t *>(mem) = real_size;
  return static_cast<std::byte *>(mem) + MALLOC_REPLACER_SIZE_OFFSET;
}

inline void *calloc(size_t nmemb, size_t size) noexcept {
  void *res{alloc(nmemb * size)};
  if (res == nullptr) [[unlikely]] {
    return nullptr;
  }
  return memset(res, 0, nmemb * size);
}

inline void *realloc(void *p, size_t new_size) noexcept {
  if (p == nullptr) [[unlikely]] {
    return alloc(new_size);
  }

  if (new_size == 0) [[unlikely]] {
    free(p);
    return nullptr;
  }

  void *real_p{static_cast<std::byte *>(p) - sizeof(size_t)};
  const size_t old_size{*static_cast<size_t *>(real_p)};

  void *new_p{alloc(new_size)};
  if (new_p != nullptr) {
    memcpy(new_p, p, std::min(new_size, old_size));
    RuntimeAllocator::get().free_script_memory(real_p, old_size);
  }
  return new_p;
}

inline void free(void *mem) noexcept {
  if (mem) {
    mem = static_cast<std::byte *>(mem) - MALLOC_REPLACER_SIZE_OFFSET;
    RuntimeAllocator::get().free_script_memory(mem, *static_cast<size_t *>(mem));
  }
}

} // namespace malloc_replace

} // namespace kphp
