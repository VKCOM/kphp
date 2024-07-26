// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <concepts>
#include <coroutine>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

#include "runtime-core/utils/kphp-assert-core.h"
#include "runtime-light/component/component.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/fork/fork-context.h"
#include "runtime-light/fork/fork.h"
#include "runtime-light/header.h"
#include "runtime-light/scheduler/scheduler.h"
#include "runtime-light/utils/context.h"

template<class T>
concept Awaitable = requires(T && awaitable, std::coroutine_handle<> coro) {
  { awaitable.await_ready() } noexcept -> std::convertible_to<bool>;
  { awaitable.await_suspend(coro) } noexcept;
  { awaitable.await_resume() } noexcept;
};

template<class T>
concept CancellableAwaitable = Awaitable<T> && requires(T && awaitable) {
  { awaitable.cancel() } noexcept -> std::same_as<void>;
};

// === Awaitables =================================================================================

struct wait_for_update_t {
  uint64_t stream_d;

  explicit wait_for_update_t(uint64_t stream_d_) noexcept
    : stream_d(stream_d_) {}

  bool await_ready() const noexcept {
    return get_component_context()->stream_updated(stream_d);
  }

  void await_suspend(std::coroutine_handle<> h) const noexcept {
    CoroutineScheduler::get().suspend(h, WaitEvent::UpdateOnStream{.stream_d = stream_d});
  }

  constexpr void await_resume() const noexcept {}
};

// ================================================================================================

struct wait_for_incoming_stream_t {
  bool await_ready() const noexcept {
    return !get_component_context()->incoming_streams().empty();
  }

  void await_suspend(std::coroutine_handle<> h) const noexcept {
    CoroutineScheduler::get().suspend(h, WaitEvent::IncomingStream{});
  }

  uint64_t await_resume() const noexcept {
    const auto incoming_stream_d{get_component_context()->take_incoming_stream()};
    php_assert(incoming_stream_d != INVALID_PLATFORM_DESCRIPTOR);
    return incoming_stream_d;
  }
};

// ================================================================================================

struct wait_for_reschedule_t {
  constexpr bool await_ready() const noexcept {
    return false;
  }

  void await_suspend(std::coroutine_handle<> h) const noexcept {
    CoroutineScheduler::get().suspend(h, WaitEvent::Rechedule{});
  }

  constexpr void await_resume() const noexcept {}
};

// ================================================================================================

template<std::invocable callback_t>
class wait_for_timer_t {
  uint64_t timer_d{};
  callback_t callback;

public:
  explicit wait_for_timer_t(
    std::chrono::nanoseconds duration, callback_t &&callback_ = [] {}) noexcept
    : timer_d(get_component_context()->set_timer(duration))
    , callback(std::move(callback_)) {}

  bool await_ready() const noexcept {
    TimePoint tp{};
    return timer_d == INVALID_PLATFORM_DESCRIPTOR || get_platform_context()->get_timer_status(timer_d, std::addressof(tp)) == TimerStatus::TimerStatusElapsed;
  }

  void await_suspend(std::coroutine_handle<> h) const noexcept {
    CoroutineScheduler::get().suspend(h, WaitEvent::UpdateOnTimer{.timer_d = timer_d});
  }

  auto await_resume() const noexcept {
    get_component_context()->release_stream(timer_d);
    return std::invoke(std::move(callback));
  }
};

// ================================================================================================

class start_fork_and_reschedule_t {
  std::coroutine_handle<> coro;
  int64_t fork_id{};

public:
  explicit start_fork_and_reschedule_t(task_t<fork_result> &&task_) noexcept
    : coro(task_.get_handle())
    , fork_id(ForkComponentContext::get().register_fork(std::move(task_))) {}

  constexpr bool await_ready() const noexcept {
    return false;
  }

  std::coroutine_handle<> await_suspend(std::coroutine_handle<> h) noexcept {
    CoroutineScheduler::get().suspend(h, WaitEvent::Rechedule{});
    return coro;
  }

  int64_t await_resume() const noexcept {
    return fork_id;
  }
};
