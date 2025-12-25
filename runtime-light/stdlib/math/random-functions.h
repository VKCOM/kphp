// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <memory>
#include <random>
#include <utility>

#if defined(__APPLE__)
#include <stdlib.h>
#else
#include <sys/random.h>
#endif

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/math/random-functions.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/math/random-state.h"
#include "runtime-light/stdlib/system/system-functions.h"

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

namespace random_impl_ {

inline constexpr int64_t lcg_prime1 = (static_cast<int64_t>(1) << 31) - static_cast<int64_t>(85);
inline constexpr int64_t a1 = 53668;
inline constexpr int64_t b1 = 40014;
inline constexpr int64_t c1 = 12211;

inline constexpr int64_t lcg_prime2 = (static_cast<int64_t>(1) << 31) - static_cast<int64_t>(249);
inline constexpr int64_t a2 = 52774;
inline constexpr int64_t b2 = 40692;
inline constexpr int64_t c2 = 3791;

// invariant:   |lcg_modmult(S1)|  \in  (-P1 .. P1)
static_assert(b1 * (a1 - 1) - c1 * (0) < lcg_prime1);                 // max lcg_modmult(S1) < P1
static_assert(b1 * 0 - c1 * ((lcg_prime1 - 1) / a1) > (-lcg_prime1)); // min lcg_modmult(S1) > -P1

// invariant:   |lcg_modmult(S2)|  \in  (-P2 .. P2)
static_assert(b2 * (a2 - 1) - c2 * (0) < lcg_prime2);                 // max lcg_modmult(S2) < P2
static_assert(b2 * 0 - c2 * ((lcg_prime2 - 1) / a2) > (-lcg_prime2)); // min lcg_modmult(S2) > -P2

inline constexpr double lcg_value_coef = 4.656613e-10;

inline int64_t lcg_modmult(int64_t a, int64_t b, int64_t c, int64_t m, int64_t s) noexcept {
  int64_t q{s / a};
  int64_t res{b * (s - a * q) - c * q}; // res := b * (s % a) - c * (s / a)
  if (res < 0) {
    res += m;
  }
  return res;
}

inline int64_t secure_rand_buf(void* const buf, size_t length) noexcept {
#if defined(__APPLE__)
  arc4random_buf(buf, length);
  return 0;
#else
  return getrandom(buf, length, GRND_NONBLOCK);
#endif
}

// Analogue of unix's `gettimeofday`
// Returns seconds elapsed since Epoch, and milliseconds elapsed from the last second.
inline std::pair<std::chrono::seconds, std::chrono::microseconds> system_seconds_and_micros() noexcept {
  k2::SystemTime timeval{};
  k2::system_time(std::addressof(timeval));
  std::chrono::nanoseconds nanos_since_epoch{timeval.since_epoch_ns};
  std::chrono::microseconds micros_since_epoch{std::chrono::duration_cast<std::chrono::microseconds>(nanos_since_epoch)};
  std::chrono::seconds seconds_since_epoch{std::chrono::duration_cast<std::chrono::seconds>(nanos_since_epoch)};

  std::chrono::microseconds micros_since_last_second{micros_since_epoch - std::chrono::duration_cast<std::chrono::microseconds>(seconds_since_epoch)};
  return {
      seconds_since_epoch,
      micros_since_last_second,
  };
}

} // namespace random_impl_

inline double f$lcg_value() noexcept {
  auto& random_instance_state{RandomInstanceState::get()};
  if (!random_instance_state.lcg_initialized) {
    // TODO move this init to RandomInstanceState constructor, which requires separate header for lcg_prime1 and lcg_prime2
    uint32_t tmp{};
    k2::os_rnd(sizeof(tmp), std::addressof(tmp));
    random_instance_state.lcg1 = static_cast<int64_t>(tmp) % random_impl_::lcg_prime1;
    k2::os_rnd(sizeof(tmp), std::addressof(tmp));
    random_instance_state.lcg2 = static_cast<int64_t>(tmp) % random_impl_::lcg_prime2;
    random_instance_state.lcg_initialized = true;
  }

  random_instance_state.lcg1 =
      random_impl_::lcg_modmult(random_impl_::a1, random_impl_::b1, random_impl_::c1, random_impl_::lcg_prime1, random_instance_state.lcg1);
  random_instance_state.lcg2 =
      random_impl_::lcg_modmult(random_impl_::a2, random_impl_::b2, random_impl_::c2, random_impl_::lcg_prime2, random_instance_state.lcg2);

  int64_t z{random_instance_state.lcg1 - random_instance_state.lcg2};
  if (z < 1) {
    z += std::max(random_impl_::lcg_prime1, random_impl_::lcg_prime2) - 1;
  }

  return static_cast<double>(z) * random_impl_::lcg_value_coef;
}

inline Optional<string> f$random_bytes(int64_t length) noexcept {
  if (length < 1) [[unlikely]] {
    kphp::log::warning("argument #1 ($length) must be greater than 0");
    return false;
  }

  string str{static_cast<string::size_type>(length), false};

  if (random_impl_::secure_rand_buf(static_cast<void*>(str.buffer()), static_cast<size_t>(length)) == -1) {
    kphp::log::warning("source of randomness cannot be found");
    return false;
  }

  return str;
}

inline kphp::coro::task<string> f$uniqid(string prefix = string{}, bool more_entropy = false) noexcept {
  if (!more_entropy) {
    co_await f$usleep(1);
  }

  auto [sec, susec]{random_impl_::system_seconds_and_micros()};
  auto sec_cnt{static_cast<int32_t>(sec.count() & 0xFFFFFFFF)};  // because we'll use only 8 hex digits
  auto susec_cnt{static_cast<int32_t>(susec.count() & 0xFFFFF)}; // because we'll use only 5 hex digits
  constexpr size_t buf_size = 30;
  std::array<char, buf_size> buf{};
  auto& runtime_context{RuntimeContext::get()};
  runtime_context.static_SB.clean() << prefix;

  if (more_entropy) {
    // we multiply by 10 to get (0..10) value out of (0..1), because we want random digit before the point.
    double lcg_rand_value{f$lcg_value() * 10};
    std::format_to_n(buf.data(), buf_size, "{:08x}{:05x}{:.8f}", sec_cnt, susec_cnt, lcg_rand_value);
    constexpr size_t rand_len = 23;
    runtime_context.static_SB.append(buf.data(), rand_len);
  } else {
    std::format_to_n(buf.data(), buf_size, "{:08x}{:05x}", sec_cnt, susec_cnt);
    constexpr size_t rand_len = 13;
    runtime_context.static_SB.append(buf.data(), rand_len);
  }

  co_return runtime_context.static_SB.str();
}
