// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "runtime-common/stdlib/diagnostics/builtin-time-stats.h"

enum class RegexBuiltin : uint8_t {
  preg_match,
  preg_match_all,
  preg_replace,
  preg_replace_callback,
  preg_split,
  preg_quote,
  count,
};

inline constexpr std::array<std::string_view, static_cast<size_t>(RegexBuiltin::count)> REGEX_BUILTIN_NAMES{
    "preg_match", "preg_match_all", "preg_replace", "preg_replace_callback", "preg_split", "preg_quote",
};

// Accumulates time spent in regex builtins (preg_*) so it can be periodically reported to StatsHouse.
// `get()` is provided separately by each runtime: the legacy runtime returns a process-wide static
// instance, the K2 runtime returns a per-instance one stored in InstanceState.
struct RegexTimeStats final : BuiltinTimeStats<RegexBuiltin, static_cast<size_t>(RegexBuiltin::count)> {
  static RegexTimeStats& get() noexcept;
};
