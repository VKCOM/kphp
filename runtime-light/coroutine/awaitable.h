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

#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/component/component.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/scheduler/scheduler.h"
#include "runtime-light/stdlib/fork/fork-context.h"

template<class T>
concept Awaitable = requires(T awaitable, std::coroutine_handle<> coro) {
  { awaitable.await_ready() } noexcept -> std::convertible_to<bool>;
  { awaitable.await_suspend(coro) } noexcept;
  { awaitable.await_resume() } noexcept;
};

template<class T>
concept CancellableAwaitable = Awaitable<T> && requires(T awaitable) {
  // semantics: returns 'true' if it's safe to call 'await_resume', 'false' otherwise.
  { awaitable.resumable() } noexcept -> std::convertible_to<bool>;
  // semantics: following resume of the awaitable should not have any effect on the awaiter.
  // semantics (optional): stop useless calculations.
  { awaitable.cancel() } noexcept -> std::same_as<void>;
};

// === Awaitables =================================================================================
//
// ***Important***
// Below awaitables are not supposed to be co_awaited on more than once.

namespace awaitable_impl_ {

enum class State : uint8_t { Init, Suspend, Ready, End };

class fork_id_watcher_t {
  int64_t fork_id{ForkInstanceState::get().running_fork_id};

protected:
  void await_resume() const noexcept {
    ForkInstanceState::get().running_fork_id = fork_id;
  }
};

} // namespace awaitable_impl_

class wait_for_update_t : public awaitable_impl_::fork_id_watcher_t {
  uint64_t stream_d;
  SuspendToken suspend_token;
  awaitable_impl_::State state{awaitable_impl_::State::Init};

public:
  explicit wait_for_update_t(uint64_t stream_d_) noexcept
    : stream_d(stream_d_)
    , suspend_token(std::noop_coroutine(), WaitEvent::UpdateOnStream{.stream_d = stream_d}) {}

  wait_for_update_t(wait_for_update_t &&other) noexcept
    : stream_d(std::exchange(other.stream_d, INVALID_PLATFORM_DESCRIPTOR))
    , suspend_token(std::exchange(other.suspend_token, std::make_pair(std::noop_coroutine(), WaitEvent::Rechedule{})))
    , state(std::exchange(other.state, awaitable_impl_::State::End)) {}

  wait_for_update_t(const wait_for_update_t &) = delete;
  wait_for_update_t &operator=(const wait_for_update_t &) = delete;
  wait_for_update_t &operator=(wait_for_update_t &&) = delete;

  ~wait_for_update_t() {
    if (state == awaitable_impl_::State::Suspend) {
      cancel();
    }
  }

  bool await_ready() noexcept {
    php_assert(state == awaitable_impl_::State::Init);
    state = InstanceState::get().stream_updated(stream_d) ? awaitable_impl_::State::Ready : awaitable_impl_::State::Init;
    return state == awaitable_impl_::State::Ready;
  }

  void await_suspend(std::coroutine_handle<> coro) noexcept {
    state = awaitable_impl_::State::Suspend;
    suspend_token.first = coro;
    CoroutineScheduler::get().suspend(suspend_token);
  }

  constexpr void await_resume() noexcept {
    state = awaitable_impl_::State::End;
    fork_id_watcher_t::await_resume();
  }

  bool resumable() const noexcept {
    return state == awaitable_impl_::State::Ready || (state == awaitable_impl_::State::Suspend && !CoroutineScheduler::get().contains(suspend_token));
  }

  void cancel() noexcept {
    state = awaitable_impl_::State::End;
    CoroutineScheduler::get().cancel(suspend_token);
  }
};

// ================================================================================================

class wait_for_incoming_stream_t : awaitable_impl_::fork_id_watcher_t {
  SuspendToken suspend_token{std::noop_coroutine(), WaitEvent::IncomingStream{}};
  awaitable_impl_::State state{awaitable_impl_::State::Init};

public:
  wait_for_incoming_stream_t() noexcept = default;

  wait_for_incoming_stream_t(wait_for_incoming_stream_t &&other) noexcept
    : suspend_token(std::exchange(other.suspend_token, std::make_pair(std::noop_coroutine(), WaitEvent::Rechedule{})))
    , state(std::exchange(other.state, awaitable_impl_::State::End)) {}

  wait_for_incoming_stream_t(const wait_for_incoming_stream_t &) = delete;
  wait_for_incoming_stream_t &operator=(const wait_for_incoming_stream_t &) = delete;
  wait_for_incoming_stream_t &operator=(wait_for_incoming_stream_t &&) = delete;

  ~wait_for_incoming_stream_t() {
    if (state == awaitable_impl_::State::Suspend) {
      cancel();
    }
  }

  bool await_ready() noexcept {
    php_assert(state == awaitable_impl_::State::Init);
    state = !InstanceState::get().incoming_streams().empty() ? awaitable_impl_::State::Ready : awaitable_impl_::State::Init;
    return state == awaitable_impl_::State::Ready;
  }

  void await_suspend(std::coroutine_handle<> coro) noexcept {
    state = awaitable_impl_::State::Suspend;
    suspend_token.first = coro;
    CoroutineScheduler::get().suspend(suspend_token);
  }

  uint64_t await_resume() noexcept {
    state = awaitable_impl_::State::End;
    fork_id_watcher_t::await_resume();
    const auto incoming_stream_d{InstanceState::get().take_incoming_stream()};
    php_assert(incoming_stream_d != INVALID_PLATFORM_DESCRIPTOR);
    return incoming_stream_d;
  }

  bool resumable() const noexcept {
    return state == awaitable_impl_::State::Ready || (state == awaitable_impl_::State::Suspend && !CoroutineScheduler::get().contains(suspend_token));
  }

  void cancel() noexcept {
    state = awaitable_impl_::State::End;
    CoroutineScheduler::get().cancel(suspend_token);
  }
};

// ================================================================================================

class wait_for_reschedule_t : awaitable_impl_::fork_id_watcher_t {
  SuspendToken suspend_token{std::noop_coroutine(), WaitEvent::Rechedule{}};
  awaitable_impl_::State state{awaitable_impl_::State::Init};

public:
  wait_for_reschedule_t() noexcept = default;

  wait_for_reschedule_t(wait_for_reschedule_t &&other) noexcept
    : suspend_token(std::exchange(other.suspend_token, std::make_pair(std::noop_coroutine(), WaitEvent::Rechedule{})))
    , state(std::exchange(other.state, awaitable_impl_::State::End)) {}

  wait_for_reschedule_t(const wait_for_reschedule_t &) = delete;
  wait_for_reschedule_t &operator=(const wait_for_reschedule_t &) = delete;
  wait_for_reschedule_t &operator=(wait_for_reschedule_t &&) = delete;

  ~wait_for_reschedule_t() {
    if (state == awaitable_impl_::State::Suspend) {
      cancel();
    }
  }

  constexpr bool await_ready() const noexcept {
    php_assert(state == awaitable_impl_::State::Init);
    return false;
  }

  void await_suspend(std::coroutine_handle<> coro) noexcept {
    state = awaitable_impl_::State::Suspend;
    suspend_token.first = coro;
    CoroutineScheduler::get().suspend(suspend_token);
  }

  constexpr void await_resume() noexcept {
    state = awaitable_impl_::State::End;
    fork_id_watcher_t::await_resume();
  }

  bool resumable() const noexcept {
    return state == awaitable_impl_::State::Suspend && !CoroutineScheduler::get().contains(suspend_token);
  }

  void cancel() noexcept {
    state = awaitable_impl_::State::End;
    CoroutineScheduler::get().cancel(suspend_token);
  }
};

// ================================================================================================

class wait_for_timer_t : awaitable_impl_::fork_id_watcher_t {
  std::chrono::nanoseconds duration;
  uint64_t timer_d{INVALID_PLATFORM_DESCRIPTOR};
  SuspendToken suspend_token{std::noop_coroutine(), WaitEvent::Rechedule{}};
  awaitable_impl_::State state{awaitable_impl_::State::Init};

public:
  explicit wait_for_timer_t(std::chrono::nanoseconds duration_) noexcept
    : duration(duration_) {}

  wait_for_timer_t(wait_for_timer_t &&other) noexcept
    : duration(std::exchange(other.duration, std::chrono::nanoseconds{0}))
    , timer_d(std::exchange(other.timer_d, INVALID_PLATFORM_DESCRIPTOR))
    , suspend_token(std::exchange(other.suspend_token, std::make_pair(std::noop_coroutine(), WaitEvent::Rechedule{})))
    , state(std::exchange(other.state, awaitable_impl_::State::End)) {}

  wait_for_timer_t(const wait_for_timer_t &) = delete;
  wait_for_timer_t &operator=(const wait_for_timer_t &) = delete;
  wait_for_timer_t &operator=(wait_for_timer_t &&) = delete;

  ~wait_for_timer_t() {
    if (state == awaitable_impl_::State::Suspend) {
      cancel();
    }
    if (timer_d != INVALID_PLATFORM_DESCRIPTOR) {
      InstanceState::get().release_stream(timer_d);
    }
  }

  constexpr bool await_ready() const noexcept {
    php_assert(state == awaitable_impl_::State::Init);
    return false;
  }

  void await_suspend(std::coroutine_handle<> coro) noexcept {
    state = awaitable_impl_::State::Suspend;
    timer_d = InstanceState::get().set_timer(duration);
    if (timer_d != INVALID_PLATFORM_DESCRIPTOR) {
      suspend_token = std::make_pair(coro, WaitEvent::UpdateOnTimer{.timer_d = timer_d});
    }
    CoroutineScheduler::get().suspend(suspend_token);
  }

  constexpr void await_resume() noexcept {
    state = awaitable_impl_::State::End;
    fork_id_watcher_t::await_resume();
  }

  bool resumable() const noexcept {
    return state == awaitable_impl_::State::Suspend && !CoroutineScheduler::get().contains(suspend_token);
  }

  void cancel() noexcept {
    state = awaitable_impl_::State::End;
    CoroutineScheduler::get().cancel(suspend_token);
  }
};

// ================================================================================================

class start_fork_t : awaitable_impl_::fork_id_watcher_t {
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
  awaitable_impl_::State state{awaitable_impl_::State::Init};

public:
  explicit start_fork_t(task_t<void> task_, execution exec_policy_) noexcept
    : exec_policy(exec_policy_)
    , fork_coro(task_.get_handle())
    , fork_id(ForkInstanceState::get().push_fork(std::move(task_))) {}

  start_fork_t(start_fork_t &&other) noexcept
    : exec_policy(other.exec_policy)
    , fork_coro(std::exchange(other.fork_coro, std::noop_coroutine()))
    , fork_id(std::exchange(other.fork_id, INVALID_FORK_ID))
    , suspend_token(std::exchange(other.suspend_token, std::make_pair(std::noop_coroutine(), WaitEvent::Rechedule{})))
    , state(std::exchange(other.state, awaitable_impl_::State::End)) {}

  start_fork_t(const start_fork_t &) = delete;
  start_fork_t &operator=(const start_fork_t &) = delete;
  start_fork_t &operator=(start_fork_t &&) = delete;
  ~start_fork_t() = default;

  constexpr bool await_ready() const noexcept {
    php_assert(state == awaitable_impl_::State::Init);
    return false;
  }

  std::coroutine_handle<> await_suspend(std::coroutine_handle<> current_coro) noexcept {
    state = awaitable_impl_::State::Suspend;
    std::coroutine_handle<> continuation{};
    switch (exec_policy) {
      case execution::fork: {
        suspend_token.first = current_coro;
        continuation = fork_coro;
        ForkInstanceState::get().running_fork_id = fork_id;
        break;
      }
      case execution::self: {
        suspend_token.first = fork_coro;
        continuation = current_coro;
        break;
      }
    }
    CoroutineScheduler::get().suspend(suspend_token);
    // reset fork_coro and suspend_token to guarantee that the same fork will be started only once
    fork_coro = std::noop_coroutine();
    suspend_token = std::make_pair(std::noop_coroutine(), WaitEvent::Rechedule{});
    return continuation;
  }

  int64_t await_resume() noexcept {
    state = awaitable_impl_::State::End;
    fork_id_watcher_t::await_resume();
    return fork_id;
  }
};

// ================================================================================================

template<typename T>
class wait_fork_t : awaitable_impl_::fork_id_watcher_t {
  int64_t fork_id;
  task_t<T> fork_task;
  task_t<T>::awaiter_t fork_awaiter;
  awaitable_impl_::State state{awaitable_impl_::State::Init};

  using fork_resume_t = decltype(fork_awaiter.await_resume());
  using await_resume_t = fork_resume_t;

public:
  explicit wait_fork_t(int64_t fork_id_) noexcept
    : fork_id(fork_id_)
    , fork_task(static_cast<task_t<T>>(ForkInstanceState::get().pop_fork(fork_id)))
    , fork_awaiter(std::addressof(fork_task)) {}

  wait_fork_t(wait_fork_t &&other) noexcept
    : fork_id(std::exchange(other.fork_id, INVALID_FORK_ID))
    , fork_task(std::move(other.fork_task))
    , fork_awaiter(std::addressof(fork_task))
    , state(std::exchange(other.state, awaitable_impl_::State::End)) {}

  wait_fork_t(const wait_fork_t &) = delete;
  wait_fork_t &operator=(const wait_fork_t &) = delete;
  wait_fork_t &operator=(wait_fork_t &&) = delete;

  ~wait_fork_t() {
    if (state == awaitable_impl_::State::Suspend) {
      cancel();
    }
  }

  bool await_ready() noexcept {
    php_assert(state == awaitable_impl_::State::Init);
    state = fork_awaiter.resumable() ? awaitable_impl_::State::Ready : awaitable_impl_::State::Init;
    return state == awaitable_impl_::State::Ready;
  }

  constexpr void await_suspend(std::coroutine_handle<> coro) noexcept {
    state = awaitable_impl_::State::Suspend;
    fork_awaiter.await_suspend(coro);
  }

  await_resume_t await_resume() noexcept {
    state = awaitable_impl_::State::End;
    fork_id_watcher_t::await_resume();
    if constexpr (std::is_void_v<await_resume_t>) {
      fork_awaiter.await_resume();
    } else {
      return fork_awaiter.await_resume();
    }
  }

  constexpr bool resumable() const noexcept {
    return state == awaitable_impl_::State::Ready || (state == awaitable_impl_::State::Suspend && fork_awaiter.resumable());
  }

  constexpr void cancel() noexcept {
    state = awaitable_impl_::State::End;
    fork_awaiter.cancel();
  }
};

// ================================================================================================

template<CancellableAwaitable T>
class wait_with_timeout_t {
  T awaitable;
  wait_for_timer_t timer_awaitable;
  awaitable_impl_::State state{awaitable_impl_::State::Init};

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
    , timer_awaitable(std::move(other.timer_awaitable))
    , state(std::exchange(other.state, awaitable_impl_::State::End)) {}

  wait_with_timeout_t(const wait_with_timeout_t &) = delete;
  wait_with_timeout_t &operator=(const wait_with_timeout_t &) = delete;
  wait_with_timeout_t &operator=(wait_with_timeout_t &&) = delete;
  ~wait_with_timeout_t() = default;

  constexpr bool await_ready() noexcept {
    php_assert(state == awaitable_impl_::State::Init);
    state = awaitable.await_ready() || timer_awaitable.await_ready() ? awaitable_impl_::State::Ready : awaitable_impl_::State::Init;
    return state == awaitable_impl_::State::Ready;
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
    state = awaitable_impl_::State::Suspend;
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
    state = awaitable_impl_::State::End;
    if (awaitable.resumable()) {
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
