// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include "runtime-common/stdlib/diagnostics/builtin-time-stats.h"
#include <algorithm>
#include <chrono>
#include <optional>
#include <vector>

// One async transfer's network wait. Ready-but-not-dispatched sockets keep
// the timer running: the reactor's queue is algorithm overhead, not network.
class CurlNetworkWait {
public:
  void begin() {
    if (ready_fds_.empty() && !started_) {
      started_ = BuiltinTimeClock::now();
    }
  }
  void end() {
    if (started_) {
      elapsed_ns_ += static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(BuiltinTimeClock::now() - *started_).count());
      started_.reset();
    }
  }
  void ready(int fd) {
    end();
    if (std::find(ready_fds_.begin(), ready_fds_.end(), fd) == ready_fds_.end()) {
      ready_fds_.push_back(fd);
    }
  }
  void consume(int fd) {
    end();
    ready_fds_.erase(std::remove(ready_fds_.begin(), ready_fds_.end(), fd), ready_fds_.end());
  }
  uint64_t elapsed_ns() const {
    return elapsed_ns_;
  }

private:
  uint64_t elapsed_ns_{0};
  std::optional<std::chrono::steady_clock::time_point> started_;
  std::vector<int> ready_fds_;
};
