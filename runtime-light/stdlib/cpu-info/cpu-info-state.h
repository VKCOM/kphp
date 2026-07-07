// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#if defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>
#endif

#include "common/containers/final_action.h"

class CpuInfoInstanceState {
private:
  [[gnu::always_inline]] static uint64_t rdtsc() noexcept {
#if defined(__x86_64__) || defined(__i386__)
    return __rdtsc();
#else
    return 0;
#endif
  }

public:
  CpuInfoInstanceState() noexcept = default;

  static CpuInfoInstanceState& get() noexcept;

  [[nodiscard]] static auto write_cycles(uint64_t& cycles_accumulator) noexcept {
    uint64_t start{CpuInfoInstanceState::rdtsc()};
    return vk::final_action([start, &cycles_accumulator]() noexcept { cycles_accumulator += CpuInfoInstanceState::rdtsc() - start; });
  }

  uint64_t processing_cycles{0};
  uint64_t coro_alloc_cycles{0};
  uint64_t coro_free_cycles{0};
};
