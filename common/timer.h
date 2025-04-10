// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>

namespace vk {

template<class Clock, class Duration>
class Timer {
public:
  using TimePoint = std::chrono::time_point<Clock, Duration>;

  void start() {
    start_tp = now();
    started = true;
  }

  auto time() const {
    return now() - start_tp;
  }

  bool is_started() const {
    return started;
  }

  void reset() {
    started = false;
    start_tp = {};
  }

private:
  TimePoint start_tp;
  bool started{false};

  static auto now() {
    return std::chrono::time_point_cast<Duration>(Clock::now());
  }
};

template<typename Duration>
using SteadyTimer = Timer<std::chrono::steady_clock, Duration>;

} // namespace vk
