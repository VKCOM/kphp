// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/allocator/runtime-allocator.h"

inline int64_t f$memory_get_peak_usage(bool real_usage = false) noexcept {
  if (real_usage) {
    return static_cast<int64_t>(RuntimeAllocator::get().memory_resource.get_memory_stats().max_real_memory_used);
  } else {
    return static_cast<int64_t>(RuntimeAllocator::get().memory_resource.get_memory_stats().max_memory_used);
  }
}

inline int64_t f$memory_get_usage([[maybe_unused]] bool real_usage = false) noexcept {
  return static_cast<int64_t>(RuntimeAllocator::get().memory_resource.get_memory_stats().memory_used);
}
