// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <concepts>
#include <coroutine>
#include <cstdint>
#include <memory>
#include <utility>

#include "runtime-core/core-types/decl/optional.h"
#include "runtime-core/utils/kphp-assert-core.h"
#include "runtime-light/component/component.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/fork/fork-context.h"
#include "runtime-light/stdlib/fork/fork.h"
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

class wait_for_update_t {
  uint64_t stream_d;
  SuspendToken suspend_token_;
  uint64_t suspended_fork_id_{};

public:
  explicit wait_for_update_t(uint64_t stream_d_) noexcept
    : stream_d(stream_d_)
    , suspend_token_(std::noop_coroutine(), WaitEvent::UpdateOnStream{.stream_d = stream_d}) {}

  bool await_ready() const noexcept {
    return get_component_context()->stream_updated(stream_d);
  }

  void await_suspend(std::coroutine_handle<> coro) noexcept {
    suspend_token_.first = coro;
    CoroutineScheduler::get().suspend(suspend_token_);
    suspended_fork_id_ = ForkComponentContext::get().get_running_fork_id();
  }

  void await_resume() const noexcept {
    ForkComponentContext::get().set_running_fork_id(suspended_fork_id_);
  }

  void cancel() const noexcept {
    CoroutineScheduler::get().cancel(suspend_token_);
  }
};

// ================================================================================================

class wait_for_incoming_stream_t {
  SuspendToken suspend_token_{std::noop_coroutine(), WaitEvent::IncomingStream{}};
  uint64_t suspended_fork_id_{};

public:
  bool await_ready() const noexcept {
    return !get_component_context()->incoming_streams().empty();
  }

  void await_suspend(std::coroutine_handle<> coro) noexcept {
    suspend_token_.first = coro;
    CoroutineScheduler::get().suspend(suspend_token_);
    suspended_fork_id_ = ForkComponentContext::get().get_running_fork_id();
  }

  uint64_t await_resume() const noexcept {
    ForkComponentContext::get().set_running_fork_id(suspended_fork_id_);
    const auto incoming_stream_d{get_component_context()->take_incoming_stream()};
    php_assert(incoming_stream_d != INVALID_PLATFORM_DESCRIPTOR);
    return incoming_stream_d;
  }

  void cancel() const noexcept {
    CoroutineScheduler::get().cancel(suspend_token_);
  }
};

// ================================================================================================

class wait_for_reschedule_t {
  SuspendToken suspend_token_{std::noop_coroutine(), WaitEvent::Rechedule{}};
  uint64_t suspended_fork_id_{};

public:
  constexpr bool await_ready() const noexcept {
    return false;
  }

  void await_suspend(std::coroutine_handle<> coro) noexcept {
    suspend_token_.first = coro;
    CoroutineScheduler::get().suspend(suspend_token_);
    suspended_fork_id_ = ForkComponentContext::get().get_running_fork_id();
  }

  void await_resume() const noexcept {
    ForkComponentContext::get().set_running_fork_id(suspended_fork_id_);
  }

  void cancel() const noexcept {
    CoroutineScheduler::get().cancel(suspend_token_);
  }
};

// ================================================================================================

class wait_for_timer_t {
  uint64_t timer_d{};
  SuspendToken suspend_token_;
  uint64_t suspended_fork_id_{};

public:
  explicit wait_for_timer_t(std::chrono::nanoseconds duration) noexcept
    : timer_d(get_component_context()->set_timer(duration))
    , suspend_token_(std::noop_coroutine(), WaitEvent::UpdateOnTimer{.timer_d = timer_d}) {}

  bool await_ready() const noexcept {
    TimePoint tp{};
    return timer_d == INVALID_PLATFORM_DESCRIPTOR || get_platform_context()->get_timer_status(timer_d, std::addressof(tp)) == TimerStatus::TimerStatusElapsed;
  }

  void await_suspend(std::coroutine_handle<> coro) noexcept {
    suspend_token_.first = coro;
    CoroutineScheduler::get().suspend(suspend_token_);
    suspended_fork_id_ = ForkComponentContext::get().get_running_fork_id();
  }

  void await_resume() const noexcept {
    ForkComponentContext::get().set_running_fork_id(suspended_fork_id_);
    get_component_context()->release_stream(timer_d);
  }

  void cancel() const noexcept {
    get_component_context()->release_stream(timer_d);
    CoroutineScheduler::get().cancel(suspend_token_);
  }
};

// ================================================================================================

class start_fork_and_reschedule_t {
  std::coroutine_handle<> fork_coro;
  int64_t fork_id{};
  SuspendToken suspend_token_{std::noop_coroutine(), WaitEvent::Rechedule{}};
  int64_t suspend_fork_id_{};

public:
  explicit start_fork_and_reschedule_t(task_t<fork_result> &&task_) noexcept
    : fork_coro(task_.get_handle())
    , fork_id(ForkComponentContext::get().push_fork(std::move(task_))) {}

  constexpr bool await_ready() const noexcept {
    return false;
  }

  std::coroutine_handle<> await_suspend(std::coroutine_handle<> current_coro) noexcept {
    suspend_fork_id_ = ForkComponentContext::get().get_running_fork_id();
    suspend_token_.first = current_coro;
    CoroutineScheduler::get().suspend(suspend_token_);
    ForkComponentContext::get().set_running_fork_id(fork_id);
    return fork_coro;
  }

  int64_t await_resume() const noexcept {
    ForkComponentContext::get().set_running_fork_id(suspend_fork_id_);
    return fork_id;
  }
};

// ================================================================================================

template<typename T>
class wait_fork_t {
  task_t<fork_result> task;
  wait_for_timer_t timer_awaiter;
  task_t<fork_result>::awaiter_t fork_awaiter;
  uint64_t suspended_fork_id_{};

public:
  wait_fork_t(task_t<fork_result> &&task_, std::chrono::nanoseconds timeout_) noexcept
    : task(std::move(task_))
    , timer_awaiter(timeout_)
    , fork_awaiter(std::addressof(task)) {}

  bool await_ready() const noexcept {
    return task.done();
  }

  void await_suspend(std::coroutine_handle<> coro) noexcept {
    fork_awaiter.await_suspend(coro);
    timer_awaiter.await_suspend(coro);
    suspended_fork_id_ = ForkComponentContext::get().get_running_fork_id();
  }

  Optional<T> await_resume() noexcept {
    ForkComponentContext::get().set_running_fork_id(suspended_fork_id_);
    if (task.done()) {
      timer_awaiter.cancel();
      return {fork_awaiter.await_resume().get_result<T>()};
    } else {
      fork_awaiter.cancel();
      return {};
    }
  }
};
