// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <x86intrin.h>

#include "common/containers/final_action.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

class CpuInfoInstanceState {
public:
  CpuInfoInstanceState() noexcept = default;

  static CpuInfoInstanceState& get() noexcept;

  [[gnu::always_inline]] static uint64_t rdtsc() noexcept {
    return __rdtsc();
  }

  static auto write_cycles(uint64_t& dist) noexcept {
    uint64_t start{CpuInfoInstanceState::rdtsc()};
    return vk::final_action([start, &dist]() noexcept { dist += CpuInfoInstanceState::rdtsc() - start; });
  }

  double get_percent(uint64_t cycles) const noexcept {
    if (cycles > total_cycles) {
      kphp::log::warning("CpuInfo. Something wrong: cycles {} > total_cycles {}", cycles, total_cycles);
    }
    return 100.0 * cycles / total_cycles;
  }

  void init() noexcept {
    total_cycles = 0;
    coro_alloc_cycles = 0;
    coro_free_cycles = 0;
  }

  uint64_t total_cycles;
  uint64_t coro_alloc_cycles;
  uint64_t coro_free_cycles;
};
