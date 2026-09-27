// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>

#include "common/mixin/not_copyable.h"

using BuiltinTimeClock = std::chrono::steady_clock;

class BuiltinTimeGuard final : vk::not_copyable {
public:
  BuiltinTimeGuard(const BuiltinTimeGuard&) = delete;
  BuiltinTimeGuard& operator=(const BuiltinTimeGuard&) = delete;
  BuiltinTimeGuard& operator=(BuiltinTimeGuard&&) = delete;

  BuiltinTimeGuard(uint64_t& total, uint64_t& method) noexcept
      : total_{&total},
        method_{&method},
        started_at_{BuiltinTimeClock::now()} {}

  BuiltinTimeGuard(BuiltinTimeGuard&& other) noexcept
      : total_{std::exchange(other.total_, nullptr)},
        method_{other.method_},
        elapsed_ns_{other.elapsed_ns_},
        excluded_ns_{other.excluded_ns_},
        started_at_{other.started_at_},
        is_running_{other.is_running_} {}

  ~BuiltinTimeGuard() {
    if (total_ != nullptr) {
      pause();
      const auto measured = elapsed_ns_ > excluded_ns_ ? elapsed_ns_ - excluded_ns_ : 0;
      *method_ += measured;
      *total_ += measured;
    }
  }

  bool pause() noexcept {
    if (is_running_) {
      elapsed_ns_ += static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(BuiltinTimeClock::now() - started_at_).count());
      is_running_ = false;
      return true;
    }
    return false;
  }

  void resume() noexcept {
    if (!is_running_) {
      started_at_ = BuiltinTimeClock::now();
      is_running_ = true;
    }
  }

  void subtract_elapsed_ns(uint64_t network_wait_ns) noexcept {
    excluded_ns_ += network_wait_ns;
  }

  uint64_t elapsed_ns() const noexcept {
    return elapsed_ns_;
  }

private:
  uint64_t* total_;
  uint64_t* method_;
  uint64_t elapsed_ns_{0};
  uint64_t excluded_ns_{0};
  std::chrono::steady_clock::time_point started_at_;
  bool is_running_{true};
};

// Attribution for synchronous network calls. This scope MUST NOT survive
// a coroutine suspension. Async curl installs it separately in each callback.
class BuiltinTimeNetworkScope final : vk::not_copyable {
public:
  explicit BuiltinTimeNetworkScope(BuiltinTimeGuard& timer) noexcept
      : previous_(std::exchange(current_, &timer)) {}
  ~BuiltinTimeNetworkScope() {
    current_ = previous_;
  }
  static BuiltinTimeGuard* current() noexcept {
    return current_;
  }
  static void reset() noexcept {
    current_ = nullptr;
  } // legacy fatal longjmp

private:
  inline static thread_local BuiltinTimeGuard* current_{nullptr};
  BuiltinTimeGuard* previous_;
};

class BuiltinTimePauseGuard final : vk::not_copyable {
public:
  BuiltinTimePauseGuard(const BuiltinTimePauseGuard&) = delete;
  BuiltinTimePauseGuard(BuiltinTimePauseGuard&&) = delete;
  BuiltinTimePauseGuard& operator=(const BuiltinTimePauseGuard&) = delete;
  BuiltinTimePauseGuard& operator=(BuiltinTimePauseGuard&&) = delete;

  explicit BuiltinTimePauseGuard(BuiltinTimeGuard& timer) noexcept
      : timer_{timer},
        should_resume_{timer_.pause()} {}

  ~BuiltinTimePauseGuard() {
    if (should_resume_) {
      timer_.resume();
    }
  }

private:
  BuiltinTimeGuard& timer_;
  bool should_resume_;
};

template<typename F>
auto call_with_paused_builtin_timer(BuiltinTimeGuard& timer, F&& f) noexcept(noexcept(std::invoke(std::forward<F>(f)))) -> std::invoke_result_t<F> {
  BuiltinTimePauseGuard pause_guard{timer};
  return std::invoke(std::forward<F>(f));
}

template<typename Method, size_t MethodCount>
struct BuiltinTimeStats : vk::not_copyable {

  uint64_t total{0};
  std::array<uint64_t, MethodCount> methods{};

  BuiltinTimeGuard write(Method method) noexcept {
    return {total, methods[static_cast<size_t>(method)]};
  }

  void reset() noexcept {
    total = 0;
    methods.fill(0);
  }
};
