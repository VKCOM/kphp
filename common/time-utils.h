// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>

namespace vk {
namespace time {

using namespace std::chrono_literals;

using Ms = std::chrono::milliseconds;
using Sec = std::chrono::seconds;

inline auto get_steady_tp_ms_now() noexcept {
  return std::chrono::time_point_cast<Ms>(std::chrono::steady_clock::now());
}

template<class Clock>
class Timer {
public:
  template<class Duration>
  using TimePoint = std::chrono::time_point<Clock, Duration>;

  void start() {
    start_tp = now_ms();
  }

  auto time() const {
    return now_ms() - start_tp;
  }

  bool is_started() const {
    return start_tp.time_since_epoch().count();
  }

  void reset() {
    start_tp = {};
  }

private:
  TimePoint<Ms> start_tp;

  static auto now_ms() {
    return std::chrono::time_point_cast<Ms>(Clock::now());
  }
};

using SteadyTimer = Timer<std::chrono::steady_clock>;

} // namespace time
} // namespace vk
