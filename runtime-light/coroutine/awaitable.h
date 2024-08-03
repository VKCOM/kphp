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
#include "runtime-light/header.h"
#include "runtime-light/scheduler/scheduler.h"
#include "runtime-light/stdlib/fork/fork-context.h"
#include "runtime-light/stdlib/fork/fork.h"
#include "runtime-light/utils/context.h"

template<class T>
concept Awaitable = requires(T awaitable, const T const_awaitable, std::coroutine_handle<> coro) {
  { const_awaitable.await_ready() } noexcept -> std::convertible_to<bool>;
  { awaitable.await_suspend(coro) } noexcept;
  { awaitable.await_resume() } noexcept;
};

// === Awaitables =================================================================================

class wait_for_update_t {
  uint64_t stream_d;
  SuspendToken suspend_token;

public:
  explicit wait_for_update_t(uint64_t stream_d_) noexcept
    : stream_d(stream_d_)
    , suspend_token(std::noop_coroutine(), WaitEvent::UpdateOnStream{.stream_d = stream_d}) {}

  bool await_ready() const noexcept {
    return get_component_context()->stream_updated(stream_d);
  }

  void await_suspend(std::coroutine_handle<> coro) noexcept {
    suspend_token.first = coro;
    CoroutineScheduler::get().suspend(suspend_token);
  }

  constexpr void await_resume() const noexcept {}

  void cancel() const noexcept {
    CoroutineScheduler::get().cancel(suspend_token);
  }
};

// ================================================================================================

class wait_for_incoming_stream_t {
  SuspendToken suspend_token{std::noop_coroutine(), WaitEvent::IncomingStream{}};

public:
  bool await_ready() const noexcept {
    return !get_component_context()->incoming_streams().empty();
  }

  void await_suspend(std::coroutine_handle<> coro) noexcept {
    suspend_token.first = coro;
    CoroutineScheduler::get().suspend(suspend_token);
  }

  uint64_t await_resume() const noexcept {
    const auto incoming_stream_d{get_component_context()->take_incoming_stream()};
    php_assert(incoming_stream_d != INVALID_PLATFORM_DESCRIPTOR);
    return incoming_stream_d;
  }

  void cancel() const noexcept {
    CoroutineScheduler::get().cancel(suspend_token);
  }
};

// ================================================================================================

class wait_for_reschedule_t {
  SuspendToken suspend_token{std::noop_coroutine(), WaitEvent::Rechedule{}};

public:
  constexpr bool await_ready() const noexcept {
    return false;
  }

  void await_suspend(std::coroutine_handle<> coro) noexcept {
    suspend_token.first = coro;
    CoroutineScheduler::get().suspend(suspend_token);
  }

  constexpr void await_resume() const noexcept {}

  void cancel() const noexcept {
    CoroutineScheduler::get().cancel(suspend_token);
  }
};

// ================================================================================================

class wait_for_timer_t {
  uint64_t timer_d{};
  SuspendToken suspend_token;

public:
  explicit wait_for_timer_t(std::chrono::nanoseconds duration) noexcept
    : timer_d(get_component_context()->set_timer(duration))
    , suspend_token(std::noop_coroutine(), WaitEvent::UpdateOnTimer{.timer_d = timer_d}) {}

  bool await_ready() const noexcept {
    TimePoint tp{};
    return timer_d == INVALID_PLATFORM_DESCRIPTOR || get_platform_context()->get_timer_status(timer_d, std::addressof(tp)) == TimerStatus::TimerStatusElapsed;
  }

  void await_suspend(std::coroutine_handle<> coro) noexcept {
    suspend_token.first = coro;
    CoroutineScheduler::get().suspend(suspend_token);
  }

  void await_resume() const noexcept {
    get_component_context()->release_stream(timer_d);
  }

  void cancel() const noexcept {
    get_component_context()->release_stream(timer_d);
    CoroutineScheduler::get().cancel(suspend_token);
  }
};

// ================================================================================================

class start_fork_t {
public:
  /**
   * Fork start policy:
   * 1. self: create fork, suspend fork coroutine, continue current coroutine;
   * 2. fork: create fork, suspend current coroutine, continue fork coroutine.
   */
  enum class execution : uint8_t { self, fork };

private:
  execution exec_policy;
  std::coroutine_handle<> fork_coro;
  int64_t fork_id{};
  SuspendToken suspend_token{std::noop_coroutine(), WaitEvent::Rechedule{}};

public:
  explicit start_fork_t(task_t<fork_result> &&task_, execution exec_policy_) noexcept
    : exec_policy(exec_policy_)
    , fork_coro(task_.get_handle())
    , fork_id(ForkComponentContext::get().push_fork(std::move(task_))) {}

  constexpr bool await_ready() const noexcept {
    return false;
  }

  std::coroutine_handle<> await_suspend(std::coroutine_handle<> current_coro) noexcept {
    std::coroutine_handle<> continuation{};
    switch (exec_policy) {
      case execution::fork: {
        suspend_token.first = current_coro;
        continuation = fork_coro;
        break;
      }
      case execution::self: {
        suspend_token.first = fork_coro;
        continuation = current_coro;
        break;
      }
    }
    CoroutineScheduler::get().suspend(suspend_token);
    return continuation;
  }

  constexpr int64_t await_resume() const noexcept {
    return fork_id;
  }
};

// ================================================================================================

template<typename T>
class wait_fork_t {
  int64_t fork_id;
  task_t<fork_result>::awaiter_t fork_awaiter;
  wait_for_timer_t timer_awaiter;

public:
  wait_fork_t(int64_t fork_id_, std::chrono::nanoseconds timeout) noexcept
    : fork_id(fork_id_)
    , fork_awaiter(std::addressof(ForkComponentContext::get().get_fork(fork_id)))
    , timer_awaiter(timeout) {}

  bool await_ready() const noexcept {
    return ForkComponentContext::get().get_fork(fork_id).done();
  }

  void await_suspend(std::coroutine_handle<> coro) noexcept {
    fork_awaiter.await_suspend(coro);
    timer_awaiter.await_suspend(coro);
  }

  Optional<T> await_resume() noexcept {
    if (ForkComponentContext::get().get_fork(fork_id).done()) {
      timer_awaiter.cancel();
      return {fork_awaiter.await_resume().get_result<T>()};
    } else {
      fork_awaiter.cancel();
      return {};
    }
  }
};
