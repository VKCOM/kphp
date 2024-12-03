// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <limits>
#include <memory>
#include <random>

#include "runtime-common/stdlib/math/random-functions.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/math/random-state.h"

inline int64_t f$mt_rand(int64_t l, int64_t r) noexcept {
  if (l > r) [[unlikely]] {
    return 0;
  }
  return std::uniform_int_distribution<int64_t>{l, r}(RandomInstanceState::get().mt_gen);
}

inline int64_t f$mt_rand() noexcept {
  return f$mt_rand(0, f$mt_getrandmax());
}

inline void f$mt_srand(int64_t seed = std::numeric_limits<int64_t>::min()) noexcept {
  if (seed == std::numeric_limits<int64_t>::min()) {
    k2::os_rnd(sizeof(seed), std::addressof(seed));
  }
  RandomInstanceState::get().mt_gen.seed(static_cast<std::mt19937_64::result_type>(seed));
}
