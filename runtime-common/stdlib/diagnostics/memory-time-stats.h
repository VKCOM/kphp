// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "runtime-common/stdlib/diagnostics/builtin-time-stats.h"

enum class MemoryBuiltin : uint8_t {
  task_alloc,
  task_free,
  count,
};

inline constexpr std::array<std::string_view, static_cast<size_t>(MemoryBuiltin::count)> MEMORY_BUILTIN_NAMES{
    "task_alloc",
    "task_free",
};

struct MemoryTimeStats final : BuiltinTimeStats<MemoryBuiltin, static_cast<size_t>(MemoryBuiltin::count)> {
  static MemoryTimeStats& get() noexcept;
};
