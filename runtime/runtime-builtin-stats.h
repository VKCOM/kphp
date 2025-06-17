// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"

#define SAVE_BUILTIN_CALL_STATS(builtin_name, builtin_call)                                                                                                    \
  (!runtime_builtins_stats::impl_::is_request_stats_enabled ? 0 : (*runtime_builtins_stats::request_stats.stats)[std::string_view(builtin_name)]++,            \
   builtin_call)

namespace runtime_builtins_stats {

namespace impl_ {
inline bool is_request_stats_enabled = false;
} // namespace impl_

inline bool is_server_option_enabled = false;

struct request_stats_t {
  std::optional<kphp::stl::unordered_map<std::string_view, int64_t, kphp::memory::script_allocator>> stats;
};

inline request_stats_t request_stats;

inline void init_request_stats() noexcept {
  php_assert(!request_stats.stats.has_value());
  impl_::is_request_stats_enabled = is_server_option_enabled;
  request_stats.stats.emplace();
}

inline void reset_request_stats() noexcept {
  php_assert(request_stats.stats.has_value());
  impl_::is_request_stats_enabled = false;
  request_stats.stats.reset();
}

} // namespace runtime_builtins_stats
