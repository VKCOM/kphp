// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <tuple>

#include "runtime-common/core/runtime-core.h"
#include "runtime/allocator.h"

array<int64_t> f$get_global_vars_memory_stats(int64_t lower_bound = 0);

inline int64_t f$memory_get_static_usage() {
  return static_cast<int64_t>(dl::get_heap_memory_used());
}

inline int64_t f$memory_get_peak_usage(bool real_usage = false) {
  if (real_usage) {
    return static_cast<int64_t>(dl::get_script_memory_stats().max_real_memory_used);
  } else {
    return static_cast<int64_t>(dl::get_script_memory_stats().max_memory_used);
  }
}

inline int64_t f$memory_get_usage(bool real_usage __attribute__((unused)) = false) {
  return static_cast<int64_t>(dl::get_script_memory_stats().memory_used);
}

inline int64_t f$memory_get_total_usage() {
  return static_cast<int64_t>(dl::get_script_memory_stats().real_memory_used);
}

inline array<int64_t> f$memory_get_detailed_stats() {
  const auto &stats = dl::get_script_memory_stats();
  return array<int64_t>(
    {
      std::make_pair(string{"memory_limit"}, static_cast<int64_t>(stats.memory_limit)),
      std::make_pair(string{"real_memory_used"}, static_cast<int64_t>(stats.real_memory_used)),
      std::make_pair(string{"memory_used"}, static_cast<int64_t>(stats.memory_used)),
      std::make_pair(string{"max_real_memory_used"}, static_cast<int64_t>(stats.max_real_memory_used)),
      std::make_pair(string{"max_memory_used"}, static_cast<int64_t>(stats.max_memory_used)),
      std::make_pair(string{"defragmentation_calls"}, static_cast<int64_t>(stats.defragmentation_calls)),
      std::make_pair(string{"huge_memory_pieces"}, static_cast<int64_t>(stats.huge_memory_pieces)),
      std::make_pair(string{"small_memory_pieces"}, static_cast<int64_t>(stats.small_memory_pieces)),
      std::make_pair(string{"heap_memory_used"}, static_cast<int64_t>(dl::get_heap_memory_used()))
    });
}

inline std::tuple<int64_t, int64_t> f$memory_get_allocations() {
  const auto &stats = dl::get_script_memory_stats();
  return {stats.total_allocations, stats.total_memory_allocated};
}
