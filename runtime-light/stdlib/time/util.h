// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>
#include <chrono>

#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/metaprogramming/concepts.h"

/**
 * Calculate time remaining to the deadline.
 */
template<kphp::concepts::duration duration_type>
inline duration_type deadline_to_timeout(duration_type deadline) noexcept {
  k2::TimePoint now_instant{};
  k2::instant(std::addressof(now_instant));

  std::chrono::nanoseconds now_ns{now_instant.time_point_ns};
  std::chrono::nanoseconds deadline_ns{duration_cast<std::chrono::nanoseconds>(deadline)};
  std::chrono::nanoseconds timeout_ns{deadline_ns - now_ns};

  return duration_cast<duration_type>(timeout_ns);
}

/**
 * Converts timeout to time point, when timeout will elapse - deadline.
 */
template<kphp::concepts::duration duration_type>
inline duration_type timeout_to_deadline(duration_type timeout) {
  k2::TimePoint now_instant{};
  k2::instant(std::addressof(now_instant));

  std::chrono::nanoseconds now_ns{now_instant.time_point_ns};
  std::chrono::nanoseconds timeout_ns{duration_cast<std::chrono::nanoseconds>(timeout)};
  std::chrono::nanoseconds deadline{now_ns + timeout_ns};

  return duration_cast<duration_type>(deadline);
}
