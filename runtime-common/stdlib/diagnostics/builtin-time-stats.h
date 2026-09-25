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

class BuiltinTimeGuard final : vk::not_copyable {
public:
  BuiltinTimeGuard(const BuiltinTimeGuard&) = delete;
  BuiltinTimeGuard& operator=(const BuiltinTimeGuard&) = delete;
  BuiltinTimeGuard& operator=(BuiltinTimeGuard&&) = delete;

  BuiltinTimeGuard(uint64_t& total, uint64_t& method) noexcept
      : total_{&total},
        method_{&method},
        started_at_{std::chrono::steady_clock::now()} {}

  BuiltinTimeGuard(BuiltinTimeGuard&& other) noexcept
      : total_{std::exchange(other.total_, nullptr)},
        method_{other.method_},
        elapsed_ns_{other.elapsed_ns_},
        started_at_{other.started_at_},
        is_running_{other.is_running_} {}

  ~BuiltinTimeGuard() {
    if (total_ != nullptr) {
      pause();
      *method_ += elapsed_ns_;
      *total_ += elapsed_ns_;
    }
  }

  void pause() noexcept {
    if (is_running_) {
      elapsed_ns_ += static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - started_at_).count());
      is_running_ = false;
    }
  }

  void resume() noexcept {
    if (!is_running_) {
      started_at_ = std::chrono::steady_clock::now();
      is_running_ = true;
    }
  }

private:
  uint64_t* total_;
  uint64_t* method_;
  uint64_t elapsed_ns_{0};
  std::chrono::steady_clock::time_point started_at_;
  bool is_running_{true};
};

class BuiltinTimePauseGuard final : vk::not_copyable {
public:
  BuiltinTimePauseGuard(const BuiltinTimePauseGuard&) = delete;
  BuiltinTimePauseGuard(BuiltinTimePauseGuard&&) = delete;
  BuiltinTimePauseGuard& operator=(const BuiltinTimePauseGuard&) = delete;
  BuiltinTimePauseGuard& operator=(BuiltinTimePauseGuard&&) = delete;

  explicit BuiltinTimePauseGuard(BuiltinTimeGuard& timer) noexcept
      : timer_{timer} {
    timer_.pause();
  }

  ~BuiltinTimePauseGuard() {
    timer_.resume();
  }

private:
  BuiltinTimeGuard& timer_;
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
