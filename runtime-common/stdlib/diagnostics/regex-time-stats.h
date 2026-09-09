// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <cstdint>

#include "common/containers/final_action.h"
#include "common/mixin/not_copyable.h"

// Accumulates time spent in regex builtins (preg_*) so it can be periodically reported to StatsHouse.
// `get()` is provided separately by each runtime: the legacy runtime returns a process-wide static
// instance, the K2 runtime returns a per-instance one stored in InstanceState.
struct RegexTimeStats final : vk::not_copyable {
  uint64_t total{0};
  uint64_t preg_match{0};
  uint64_t preg_match_all{0};
  uint64_t preg_replace{0};
  uint64_t preg_replace_callback{0};
  uint64_t preg_split{0};

  static RegexTimeStats& get() noexcept;

  // Usage: `auto timer{stats.write(stats.preg_match)};` at the very start of a builtin.
  // The returned guard adds the elapsed time (since this call) to both `stat` and `total` on scope exit.
  auto write(uint64_t& stat) noexcept {
    const auto start{std::chrono::steady_clock::now()};
    return vk::finally([this, &stat, start]() noexcept {
      const auto elapsed_ns{
          static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count())};
      stat += elapsed_ns;
      total += elapsed_ns;
    });
  }

  void reset() noexcept {
    total = 0;
    preg_match = 0;
    preg_match_all = 0;
    preg_replace = 0;
    preg_replace_callback = 0;
    preg_split = 0;
  }
};
