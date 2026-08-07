// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/math/random-functions.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/system/system-functions.h"

namespace {

// Analogue of unix's `gettimeofday`
// Returns seconds elapsed since Epoch, and milliseconds elapsed from the last second.
std::pair<std::chrono::seconds, std::chrono::microseconds> system_seconds_and_micros() noexcept {
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

} // namespace

kphp::coro::task<string> f$uniqid(string prefix, bool more_entropy) noexcept {
  if (!more_entropy) {
    co_await f$usleep(1);
  }

  auto [sec, susec]{system_seconds_and_micros()};
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
