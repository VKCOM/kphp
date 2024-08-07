// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <concepts>
#include <coroutine>
#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

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

template<class T>
concept CancellableAwaitable = Awaitable<T> && requires(T awaitable, const T const_awaitable) {
  // semantics: awaitable has really done its job, e.g. timer suspended and resumed, task finished etc
  { const_awaitable.done() } noexcept -> std::convertible_to<bool>;
  // semantics: following resume of the awaitable should not have any effect on the awaiter.
  // semantics (optional): stop useless calculations.
  { awaitable.cancel() } noexcept -> std::same_as<void>;
};

// === Awaitables =================================================================================

class wait_for_update_t {
  uint64_t stream_d;
  SuspendToken suspend_token;
  bool was_suspended{false};

public:
  explicit wait_for_update_t(uint64_t stream_d_) noexcept
    : stream_d(stream_d_)
    , suspend_token(std::noop_coroutine(), WaitEvent::UpdateOnStream{.stream_d = stream_d}) {}

  wait_for_update_t(wait_for_update_t &&other) noexcept
    : stream_d(std::exchange(other.stream_d, INVALID_PLATFORM_DESCRIPTOR))
    , suspend_token(std::exchange(other.suspend_token, std::make_pair(std::noop_coroutine(), WaitEvent::Rechedule{}))) {};

  wait_for_update_t(const wait_for_update_t &) = delete;
  wait_for_update_t &operator=(const wait_for_update_t &) = delete;
  wait_for_update_t &operator=(wait_for_update_t &&) = delete;

  ~wait_for_update_t() {
    if (!done()) {
      cancel();
    }
  }

  bool await_ready() const noexcept {
    return get_component_context()->stream_updated(stream_d);
  }

  void await_suspend(std::coroutine_handle<> coro) noexcept {
    was_suspended = true;
    suspend_token.first = coro;
    CoroutineScheduler::get().suspend(suspend_token);
  }

  constexpr void await_resume() const noexcept {}

  bool done() const noexcept {
    return was_suspended && !CoroutineScheduler::get().contains(suspend_token);
  }

  void cancel() const noexcept {
    CoroutineScheduler::get().cancel(suspend_token);
  }
};

// ================================================================================================

class wait_for_incoming_stream_t {
  SuspendToken suspend_token{std::noop_coroutine(), WaitEvent::IncomingStream{}};
  bool was_suspended{false};

public:
  wait_for_incoming_stream_t() noexcept = default;

  wait_for_incoming_stream_t(wait_for_incoming_stream_t &&other) noexcept
    : suspend_token(std::exchange(other.suspend_token, std::make_pair(std::noop_coroutine(), WaitEvent::Rechedule{}))) {};

  wait_for_incoming_stream_t(const wait_for_incoming_stream_t &) = delete;
  wait_for_incoming_stream_t &operator=(const wait_for_incoming_stream_t &) = delete;
  wait_for_incoming_stream_t &operator=(wait_for_incoming_stream_t &&) = delete;

  ~wait_for_incoming_stream_t() {
    if (!done()) {
      cancel();
    }
  }

  bool await_ready() const noexcept {
    return !get_component_context()->incoming_streams().empty();
  }

  void await_suspend(std::coroutine_handle<> coro) noexcept {
    was_suspended = true;
    suspend_token.first = coro;
    CoroutineScheduler::get().suspend(suspend_token);
  }

  uint64_t await_resume() const noexcept {
    const auto incoming_stream_d{get_component_context()->take_incoming_stream()};
    php_assert(incoming_stream_d != INVALID_PLATFORM_DESCRIPTOR);
    return incoming_stream_d;
  }

  bool done() const noexcept {
    return was_suspended && !CoroutineScheduler::get().contains(suspend_token);
  }

  void cancel() const noexcept {
    CoroutineScheduler::get().cancel(suspend_token);
  }
};

// ================================================================================================

class wait_for_reschedule_t {
  SuspendToken suspend_token{std::noop_coroutine(), WaitEvent::Rechedule{}};
  bool was_suspended{false};

public:
  wait_for_reschedule_t() noexcept = default;

  wait_for_reschedule_t(wait_for_reschedule_t &&other) noexcept
    : suspend_token(std::exchange(other.suspend_token, std::make_pair(std::noop_coroutine(), WaitEvent::Rechedule{}))) {};

  wait_for_reschedule_t(const wait_for_reschedule_t &) = delete;
  wait_for_reschedule_t &operator=(const wait_for_reschedule_t &) = delete;
  wait_for_reschedule_t &operator=(wait_for_reschedule_t &&) = delete;

  ~wait_for_reschedule_t() {
    if (!done()) {
      cancel();
    }
  }

  constexpr bool await_ready() const noexcept {
    return false;
  }

  void await_suspend(std::coroutine_handle<> coro) noexcept {
    was_suspended = true;
    suspend_token.first = coro;
    CoroutineScheduler::get().suspend(suspend_token);
  }

  constexpr void await_resume() const noexcept {}

  bool done() const noexcept {
    return was_suspended && !CoroutineScheduler::get().contains(suspend_token);
  }

  void cancel() const noexcept {
    CoroutineScheduler::get().cancel(suspend_token);
  }
};

// ================================================================================================

class wait_for_timer_t {
  uint64_t timer_d{};
  SuspendToken suspend_token;
  bool was_suspended{false};

public:
  explicit wait_for_timer_t(std::chrono::nanoseconds duration) noexcept
    : timer_d(get_component_context()->set_timer(duration))
    , suspend_token(std::noop_coroutine(), WaitEvent::UpdateOnTimer{.timer_d = timer_d}) {}

  wait_for_timer_t(wait_for_timer_t &&other) noexcept
    : timer_d(std::exchange(other.timer_d, INVALID_PLATFORM_DESCRIPTOR))
    , suspend_token(std::exchange(other.suspend_token, std::make_pair(std::noop_coroutine(), WaitEvent::Rechedule{}))) {}

  wait_for_timer_t(const wait_for_timer_t &) = delete;
  wait_for_timer_t &operator=(const wait_for_timer_t &) = delete;
  wait_for_timer_t &operator=(wait_for_timer_t &&) = delete;

  ~wait_for_timer_t() {
    if (!done()) {
      cancel();
    }
    if (timer_d != INVALID_PLATFORM_DESCRIPTOR) {
      get_platform_context()->free_descriptor(timer_d);
    }
  }

  bool await_ready() const noexcept {
    TimePoint tp{};
    return timer_d == INVALID_PLATFORM_DESCRIPTOR || get_platform_context()->get_timer_status(timer_d, std::addressof(tp)) == TimerStatus::TimerStatusElapsed;
  }

  void await_suspend(std::coroutine_handle<> coro) noexcept {
    was_suspended = true;
    suspend_token.first = coro;
    CoroutineScheduler::get().suspend(suspend_token);
  }

  void await_resume() const noexcept {
    get_component_context()->release_stream(timer_d);
  }

  bool done() const noexcept {
    return was_suspended && !CoroutineScheduler::get().contains(suspend_token);
  }

  void cancel() const noexcept {
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
  task_t<fork_result> fork_task;
  task_t<fork_result>::awaiter_t fork_awaiter;

public:
  explicit wait_fork_t(int64_t fork_id_) noexcept
    : fork_id(fork_id_)
    , fork_task(ForkComponentContext::get().pop_fork(fork_id))
    , fork_awaiter(std::addressof(fork_task)) {}

  wait_fork_t(wait_fork_t &&other) noexcept
    : fork_id(std::exchange(other.fork_id, INVALID_FORK_ID))
    , fork_task(std::move(other.fork_task))
    , fork_awaiter(std::addressof(fork_task)) {}

  wait_fork_t(const wait_fork_t &) = delete;
  wait_fork_t &operator=(const wait_fork_t &) = delete;
  wait_fork_t &operator=(wait_fork_t &&) = delete;
  ~wait_fork_t() = default;

  bool await_ready() const noexcept {
    return fork_awaiter.done();
  }

  constexpr void await_suspend(std::coroutine_handle<> coro) noexcept {
    fork_awaiter.await_suspend(coro);
  }

  T await_resume() noexcept {
    return fork_awaiter.await_resume().get_result<T>();
  }

  constexpr bool done() const noexcept {
    return fork_awaiter.done();
  }

  constexpr void cancel() const noexcept {
    fork_awaiter.cancel();
  }
};

// ================================================================================================

template<CancellableAwaitable T>
class wait_with_timeout_t {
  T awaitable;
  wait_for_timer_t timer_awaitable;

  static_assert(CancellableAwaitable<wait_for_timer_t>);
  static_assert(std::is_void_v<decltype(timer_awaitable.await_suspend(std::coroutine_handle<>{}))>);
  using awaitable_suspend_t = decltype(awaitable.await_suspend(std::coroutine_handle<>{}));
  using await_suspend_return_t = awaitable_suspend_t;
  using awaitable_resume_t = decltype(awaitable.await_resume());
  using await_resume_return_t = std::conditional_t<std::is_void_v<awaitable_resume_t>, void, std::optional<awaitable_resume_t>>;

public:
  wait_with_timeout_t(T &&awaitable_, std::chrono::nanoseconds timeout) noexcept
    : awaitable(std::move(awaitable_))
    , timer_awaitable(timeout) {}

  wait_with_timeout_t(wait_with_timeout_t &&other) noexcept
    : awaitable(std::move(other.awaitable))
    , timer_awaitable(std::move(other.timer_awaitable)) {}

  wait_with_timeout_t(const wait_with_timeout_t &) = delete;
  wait_with_timeout_t &operator=(const wait_with_timeout_t &) = delete;
  wait_with_timeout_t &operator=(wait_with_timeout_t &&) = delete;
  ~wait_with_timeout_t() = default;

  constexpr bool await_ready() const noexcept {
    return awaitable.await_ready() || timer_awaitable.await_ready();
  }

  // according to C++ standard, there can be 3 possible cases:
  // 1. await_suspend returns void;
  // 2. await_suspend returns bool;
  // 3. await_suspend returns std::coroutine_handle<>.
  // we must guarantee that 'co_await wait_with_timeout_t{awaitable, timeout}' behaves like 'co_await awaitable' except
  // it may cancel 'co_await awaitable' if the timeout has elapsed.
  await_suspend_return_t await_suspend(std::coroutine_handle<> coro) noexcept {
    // as we don't rely on coroutine scheduler implementation, let's always suspend awaitable first. in case of some smart scheduler
    // it won't have any effect, but it will have an effect if our scheduler is quite simple.
    if constexpr (std::is_void_v<await_suspend_return_t>) {
      awaitable.await_suspend(coro);
      timer_awaitable.await_suspend(coro);
    } else {
      const auto awaitable_suspend_res{awaitable.await_suspend(coro)};
      timer_awaitable.await_suspend(coro);
      return awaitable_suspend_res;
    }
  }

  await_resume_return_t await_resume() noexcept {
    if (awaitable.done()) {
      timer_awaitable.cancel();
      if constexpr (!std::is_void_v<await_resume_return_t>) {
        return awaitable.await_resume();
      }
    } else {
      awaitable.cancel();
      if constexpr (!std::is_void_v<await_resume_return_t>) {
        return std::nullopt;
      }
    }
  }
};

template<class T>
wait_with_timeout_t(T &&, std::chrono::nanoseconds) -> wait_with_timeout_t<T>;
