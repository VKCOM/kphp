// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <utility>

#include "runtime-common/core/allocator/runtime-allocator.h"
#include "runtime-common/core/runtime-core.h"

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

inline int64_t f$memory_get_total_usage() noexcept {
  return static_cast<int64_t>(RuntimeAllocator::get().memory_resource.get_memory_stats().real_memory_used);
}

inline array<int64_t> f$memory_get_detailed_stats() noexcept {
  const auto& stats{RuntimeAllocator::get().memory_resource.get_memory_stats()};
  return array<int64_t>({std::make_pair(string{"memory_limit"}, static_cast<int64_t>(stats.memory_limit)),
                         std::make_pair(string{"real_memory_used"}, static_cast<int64_t>(stats.real_memory_used)),
                         std::make_pair(string{"memory_used"}, static_cast<int64_t>(stats.memory_used)),
                         std::make_pair(string{"max_real_memory_used"}, static_cast<int64_t>(stats.max_real_memory_used)),
                         std::make_pair(string{"max_memory_used"}, static_cast<int64_t>(stats.max_memory_used)),
                         std::make_pair(string{"defragmentation_calls"}, static_cast<int64_t>(stats.defragmentation_calls)),
                         std::make_pair(string{"huge_memory_pieces"}, static_cast<int64_t>(stats.huge_memory_pieces)),
                         std::make_pair(string{"small_memory_pieces"}, static_cast<int64_t>(stats.small_memory_pieces)),
                         std::make_pair(string{"heap_memory_used"}, static_cast<int64_t>(0))});
}
