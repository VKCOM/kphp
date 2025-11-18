// Compiler for PHP (aka KPHP)
// TODO change to 2025 ???????????????
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <memory>
#include <random>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/math/random-functions.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/stdlib/math/random-state.h"
#include "runtime-light/stdlib/system/system-functions.h"
#include "runtime-light/stdlib/time/util.h"

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

inline int64_t f$rand() noexcept {
  return f$mt_rand();
}

inline int64_t f$rand(int64_t l, int64_t r) noexcept {
  return f$mt_rand(l, r);
}

inline void f$srand(int64_t seed = std::numeric_limits<int64_t>::min()) noexcept {
  return f$mt_srand(seed);
}

inline double f$lcg_value() noexcept {
  auto& random_instance_state{RandomInstanceState::get()};

  // TODO extract to function in namespace random_impl_ ?
  auto modmult{[](int32_t a, int32_t b, int32_t c, int32_t m, int32_t s) noexcept {
    int32_t q{s / a};
    int32_t res{b * (s - a * q) - c * q};
    if (res < 0) {
      res += m;
    }
    return res;
  }};
  // TODO where to place these magic numbers?
  random_instance_state.lcg1 = modmult(53668, 40014, 12211, 2147483563, random_instance_state.lcg1);
  random_instance_state.lcg2 = modmult(52774, 40692, 3791, 2147483399, random_instance_state.lcg2);

  int32_t z{random_instance_state.lcg1 - random_instance_state.lcg2};
  if (z < 1) {
    z += 2147483562;
  }

  return static_cast<double>(z) * 4.656613e-10;
}

inline string f$uniqid(const string& prefix, bool more_entropy) noexcept {
  if (!more_entropy) {
    // As I guess, they sleep for 1 microseconds, because f$uniqid depends on current microseconds value.
    // TODO discuss, does this sleep guarantees microseconds increment?
    // I'm sure we should replace it with monotonic clock + incrementing counter instead...
    // It seems to give more guarantees and reduce scheduler work.
    f$usleep(1);
  }

  auto [sec, susec]{system_seconds_and_micros()};
  auto sec_cnt{static_cast<int32_t>(sec.count() & 0xFFFFFFFF)};  // because we'll use only 8 hex digits
  auto susec_cnt{static_cast<int32_t>(susec.count() & 0xFFFFF)}; // because we'll use only 5 hex digits
  constexpr size_t buf_size = 30;
  std::array<char, buf_size> buf{};
  RuntimeContext::get().static_SB.clean() << prefix;

  if (more_entropy) {
    // we multiply by 10 to get (0..10) value out of (0..1), because we want random digit before the point.
    double lcg_rand_value{f$lcg_value() * 10};
    std::format_to_n(buf.data(), buf_size, "{:08x}{:05x}{:.8f}", sec_cnt, susec_cnt, lcg_rand_value);
    // TODO discuss naming and place of this constant
    constexpr size_t rand_len = 23;
    RuntimeContext::get().static_SB.append(buf.data(), rand_len);
  } else {
    std::format_to_n(buf.data(), buf_size, "{:08x}{:05x}", sec_cnt, susec_cnt);
    // TODO discuss naming and place of this constant
    constexpr size_t rand_len = 13;
    RuntimeContext::get().static_SB.append(buf.data(), rand_len);
  }

  return {
      RuntimeContext::get().static_SB.c_str(),
      RuntimeContext::get().static_SB.size(),
  };
}
