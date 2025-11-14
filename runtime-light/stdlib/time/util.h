// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <utility>

#include "runtime-light/k2-platform/k2-api.h"

// Analogue of unix's `gettimeofday`
// Returns seconds, elapsed since Epoch, and milliseconds from last second.
// TODO naming + figure out where to place this util function
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
