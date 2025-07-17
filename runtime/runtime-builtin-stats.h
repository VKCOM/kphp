// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"

namespace runtime_builtins_stats {
inline bool is_server_option_enabled = false;

struct request_stats_t {
  kphp::stl::unordered_map<std::string_view, int64_t, kphp::memory::script_allocator> builtin_stats{};
  array<int64_t> virtual_builtin_stats{};
};

inline std::optional<request_stats_t> request_stats;

inline void init_request_stats() noexcept {
  php_assert(!request_stats.has_value());
  if (is_server_option_enabled) {
    request_stats.emplace();
  }
}

inline void reset_request_stats() noexcept {
  request_stats.reset();
}

inline void save_virtual_builtin_call_stats(const string& builtin_call) noexcept {
  if (runtime_builtins_stats::request_stats.has_value()) {
    (*runtime_builtins_stats::request_stats).virtual_builtin_stats[builtin_call]++;
  }
}
} // namespace runtime_builtins_stats

#define SAVE_BUILTIN_CALL_STATS(builtin_name, builtin_call)                                                                                                    \
  (!runtime_builtins_stats::request_stats.has_value() ? 0 : (*runtime_builtins_stats::request_stats).builtin_stats[std::string_view(builtin_name)]++,          \
   builtin_call)

