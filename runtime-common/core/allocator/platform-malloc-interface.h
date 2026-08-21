// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "common/wrappers/likely.h"

namespace kphp::memory::platform {

constexpr int64_t MALLOC_REPLACER_SIZE_OFFSET = sizeof(size_t);
constexpr uint64_t MALLOC_REPLACER_MAX_ALLOC = 0xFFFFFF00;

auto alloc(size_t size) noexcept -> void*;

inline auto calloc(size_t num, size_t size) noexcept -> void* {
  void* ptr{kphp::memory::platform::alloc(num * size)};
  if (unlikely(ptr == nullptr)) {
    return nullptr;
  }

  return std::memset(ptr, 0, num * size);
}

auto free(void* ptr) noexcept -> void;

inline auto realloc(void* ptr, size_t new_size) noexcept -> void* {
  if (unlikely(ptr == nullptr)) {
    return kphp::memory::platform::alloc(new_size);
  }

  if (unlikely(new_size == 0)) {
    kphp::memory::platform::free(ptr);
    return nullptr;
  }

  void* real_ptr{static_cast<std::byte*>(ptr) - sizeof(size_t)};
  const size_t old_size{*static_cast<size_t*>(real_ptr)};

  void* new_ptr{kphp::memory::platform::alloc(new_size)};
  if (likely(new_ptr != nullptr)) {
    std::memcpy(new_ptr, ptr, std::min(new_size, old_size));
    kphp::memory::platform::free(ptr);
  }

  return new_ptr;
}

} // namespace kphp::memory::platform
