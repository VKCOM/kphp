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
#include <tuple>
#include <type_traits>
#include <utility>

#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/coroutine/coroutine-state.h"
#include "runtime-light/coroutine/shared-task.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/scheduler/scheduler.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/stdlib/fork/fork-state.h"
#include "runtime-light/utils/logs.h"

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

enum class state : uint8_t { init, suspend, ready, end };

class fork_id_watcher_t {
  int64_t fork_id{ForkInstanceState::get().current_id};

protected:
  void await_resume() const noexcept {
    ForkInstanceState::get().current_id = fork_id;
  }
};

class async_stack_watcher_t {
  kphp::coro::async_stack_root* const async_stack_root;
  kphp::coro::async_stack_frame* const suspended_async_stack_frame;

protected:
  void await_resume() const noexcept {
    async_stack_root->top_async_stack_frame = suspended_async_stack_frame;
  }

public:
  async_stack_watcher_t() noexcept
      : async_stack_root(std::addressof(CoroutineInstanceState::get().coroutine_stack_root)),
        suspended_async_stack_frame(async_stack_root->top_async_stack_frame) {}
};

} // namespace awaitable_impl_

class wait_for_update_t : awaitable_impl_::fork_id_watcher_t, awaitable_impl_::async_stack_watcher_t {
  uint64_t stream_d;
  SuspendToken suspend_token;
  awaitable_impl_::state state{awaitable_impl_::state::init};

public:
  explicit wait_for_update_t(uint64_t stream_d_) noexcept
      : stream_d(stream_d_),
        suspend_token(std::noop_coroutine(), WaitEvent::UpdateOnStream{.stream_d = stream_d}) {}

  wait_for_update_t(wait_for_update_t&& other) noexcept
      : stream_d(std::exchange(other.stream_d, k2::INVALID_PLATFORM_DESCRIPTOR)),
        suspend_token(std::exchange(other.suspend_token, std::make_pair(std::noop_coroutine(), WaitEvent::Rechedule{}))),
        state(std::exchange(other.state, awaitable_impl_::state::end)) {}

  wait_for_update_t(const wait_for_update_t&) = delete;
  wait_for_update_t& operator=(const wait_for_update_t&) = delete;
  wait_for_update_t& operator=(wait_for_update_t&&) = delete;

  ~wait_for_update_t() {
    if (state == awaitable_impl_::state::suspend) {
      cancel();
    }
  }

  bool await_ready() noexcept {
    kphp::log::assertion(state == awaitable_impl_::state::init);
    state = InstanceState::get().stream_updated(stream_d) ? awaitable_impl_::state::ready : awaitable_impl_::state::init;
    return state == awaitable_impl_::state::ready;
  }

  void await_suspend(std::coroutine_handle<> coro) noexcept {
    state = awaitable_impl_::state::suspend;
    suspend_token.first = coro;
    CoroutineScheduler::get().suspend(suspend_token);
  }

  constexpr void await_resume() noexcept {
    state = awaitable_impl_::state::end;
    async_stack_watcher_t::await_resume();
    fork_id_watcher_t::await_resume();
  }

  bool resumable() const noexcept {
    return state == awaitable_impl_::state::ready || (state == awaitable_impl_::state::suspend && !CoroutineScheduler::get().contains(suspend_token));
  }

  void cancel() noexcept {
    state = awaitable_impl_::state::end;
    CoroutineScheduler::get().cancel(suspend_token);
  }
};

// ================================================================================================

class wait_for_incoming_stream_t : awaitable_impl_::fork_id_watcher_t, awaitable_impl_::async_stack_watcher_t {
  SuspendToken suspend_token{std::noop_coroutine(), WaitEvent::IncomingStream{}};
  awaitable_impl_::state state{awaitable_impl_::state::init};

public:
  wait_for_incoming_stream_t() noexcept = default;

  wait_for_incoming_stream_t(wait_for_incoming_stream_t&& other) noexcept
      : suspend_token(std::exchange(other.suspend_token, std::make_pair(std::noop_coroutine(), WaitEvent::Rechedule{}))),
        state(std::exchange(other.state, awaitable_impl_::state::end)) {}

  wait_for_incoming_stream_t(const wait_for_incoming_stream_t&) = delete;
  wait_for_incoming_stream_t& operator=(const wait_for_incoming_stream_t&) = delete;
  wait_for_incoming_stream_t& operator=(wait_for_incoming_stream_t&&) = delete;

  ~wait_for_incoming_stream_t() {
    if (state == awaitable_impl_::state::suspend) {
      cancel();
    }
  }

  bool await_ready() noexcept {
    kphp::log::assertion(state == awaitable_impl_::state::init);
    state = !InstanceState::get().incoming_streams().empty() ? awaitable_impl_::state::ready : awaitable_impl_::state::init;
    return state == awaitable_impl_::state::ready;
  }

  void await_suspend(std::coroutine_handle<> coro) noexcept {
    state = awaitable_impl_::state::suspend;
    suspend_token.first = coro;
    CoroutineScheduler::get().suspend(suspend_token);
  }

  uint64_t await_resume() noexcept {
    state = awaitable_impl_::state::end;
    async_stack_watcher_t::await_resume();
    fork_id_watcher_t::await_resume();
    const auto incoming_stream_d{InstanceState::get().take_incoming_stream()};
    kphp::log::assertion(incoming_stream_d != k2::INVALID_PLATFORM_DESCRIPTOR);
    return incoming_stream_d;
  }

  bool resumable() const noexcept {
    return state == awaitable_impl_::state::ready || (state == awaitable_impl_::state::suspend && !CoroutineScheduler::get().contains(suspend_token));
  }

  void cancel() noexcept {
    state = awaitable_impl_::state::end;
    CoroutineScheduler::get().cancel(suspend_token);
  }
};

// ================================================================================================

class wait_for_reschedule_t : awaitable_impl_::fork_id_watcher_t, awaitable_impl_::async_stack_watcher_t {
  SuspendToken suspend_token{std::noop_coroutine(), WaitEvent::Rechedule{}};
  awaitable_impl_::state state{awaitable_impl_::state::init};

public:
  wait_for_reschedule_t() noexcept = default;

  wait_for_reschedule_t(wait_for_reschedule_t&& other) noexcept
      : suspend_token(std::exchange(other.suspend_token, std::make_pair(std::noop_coroutine(), WaitEvent::Rechedule{}))),
        state(std::exchange(other.state, awaitable_impl_::state::end)) {}

  wait_for_reschedule_t(const wait_for_reschedule_t&) = delete;
  wait_for_reschedule_t& operator=(const wait_for_reschedule_t&) = delete;
  wait_for_reschedule_t& operator=(wait_for_reschedule_t&&) = delete;

  ~wait_for_reschedule_t() {
    if (state == awaitable_impl_::state::suspend) {
      cancel();
    }
  }

  constexpr bool await_ready() const noexcept {
    kphp::log::assertion(state == awaitable_impl_::state::init);
    return false;
  }

  void await_suspend(std::coroutine_handle<> coro) noexcept {
    state = awaitable_impl_::state::suspend;
    suspend_token.first = coro;
    CoroutineScheduler::get().suspend(suspend_token);
  }

  constexpr void await_resume() noexcept {
    state = awaitable_impl_::state::end;
    async_stack_watcher_t::await_resume();
    fork_id_watcher_t::await_resume();
  }

  bool resumable() const noexcept {
    return state == awaitable_impl_::state::suspend && !CoroutineScheduler::get().contains(suspend_token);
  }

  void cancel() noexcept {
    state = awaitable_impl_::state::end;
    CoroutineScheduler::get().cancel(suspend_token);
  }
};

// ================================================================================================

class wait_for_timer_t : awaitable_impl_::fork_id_watcher_t, awaitable_impl_::async_stack_watcher_t {
  std::chrono::nanoseconds duration;
  uint64_t timer_d{k2::INVALID_PLATFORM_DESCRIPTOR};
  SuspendToken suspend_token{std::noop_coroutine(), WaitEvent::Rechedule{}};
  awaitable_impl_::state state{awaitable_impl_::state::init};

public:
  explicit wait_for_timer_t(std::chrono::nanoseconds duration_) noexcept
      : duration(duration_) {}

  wait_for_timer_t(wait_for_timer_t&& other) noexcept
      : duration(std::exchange(other.duration, std::chrono::nanoseconds{0})),
        timer_d(std::exchange(other.timer_d, k2::INVALID_PLATFORM_DESCRIPTOR)),
        suspend_token(std::exchange(other.suspend_token, std::make_pair(std::noop_coroutine(), WaitEvent::Rechedule{}))),
        state(std::exchange(other.state, awaitable_impl_::state::end)) {}

  wait_for_timer_t(const wait_for_timer_t&) = delete;
  wait_for_timer_t& operator=(const wait_for_timer_t&) = delete;
  wait_for_timer_t& operator=(wait_for_timer_t&&) = delete;

  ~wait_for_timer_t() {
    if (state == awaitable_impl_::state::suspend) {
      cancel();
    }
    if (timer_d != k2::INVALID_PLATFORM_DESCRIPTOR) {
      InstanceState::get().release_stream(timer_d);
    }
  }

  constexpr bool await_ready() const noexcept {
    kphp::log::assertion(state == awaitable_impl_::state::init);
    return false;
  }

  void await_suspend(std::coroutine_handle<> coro) noexcept {
    state = awaitable_impl_::state::suspend;
    int32_t errc{};
    std::tie(timer_d, errc) = InstanceState::get().set_timer(duration);
    if (errc == k2::errno_ok) {
      suspend_token = std::make_pair(coro, WaitEvent::UpdateOnTimer{.timer_d = timer_d});
    }
    CoroutineScheduler::get().suspend(suspend_token);
  }

  constexpr void await_resume() noexcept {
    state = awaitable_impl_::state::end;
    async_stack_watcher_t::await_resume();
    fork_id_watcher_t::await_resume();
  }

  bool resumable() const noexcept {
    return state == awaitable_impl_::state::suspend && !CoroutineScheduler::get().contains(suspend_token);
  }

  void cancel() noexcept {
    state = awaitable_impl_::state::end;
    CoroutineScheduler::get().cancel(suspend_token);
  }
};

// ================================================================================================

template<typename T>
class start_fork_t : awaitable_impl_::fork_id_watcher_t, awaitable_impl_::async_stack_watcher_t {
  ForkInstanceState& fork_instance_st{ForkInstanceState::get()};

  int64_t fork_id{};
  std::remove_cvref_t<decltype(std::declval<kphp::coro::shared_task<>>().when_ready())> fork_awaiter;
  awaitable_impl_::state state{awaitable_impl_::state::init};

public:
  explicit start_fork_t(kphp::coro::task<T>&& task) noexcept
      : fork_id(fork_instance_st.push_fork(static_cast<kphp::coro::shared_task<>>(
            std::invoke([](kphp::coro::task<T> task) noexcept -> kphp::coro::shared_task<T> { co_return co_await task; }, std::move(task))))),
        fork_awaiter((*(*fork_instance_st.get_info(fork_id)).get().opt_handle).when_ready()) {}

  start_fork_t(start_fork_t&& other) noexcept
      : fork_instance_st(other.fork_instance_st),
        fork_id(std::exchange(other.fork_id, kphp::forks::INVALID_ID)),
        fork_awaiter(std::move(other.fork_awaiter)),
        state(std::exchange(other.state, awaitable_impl_::state::end)) {}

  start_fork_t(const start_fork_t&) = delete;
  start_fork_t& operator=(const start_fork_t&) = delete;
  start_fork_t& operator=(start_fork_t&&) = delete;
  ~start_fork_t() = default;

  constexpr bool await_ready() const noexcept {
    kphp::log::assertion(state == awaitable_impl_::state::init);
    return fork_awaiter.await_ready();
  }

  template<typename promise_t>
  std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_t> current_coro) noexcept {
    state = awaitable_impl_::state::suspend;
    fork_instance_st.current_id = fork_id;
    if (fork_awaiter.await_suspend(current_coro)) [[unlikely]] {
      // If `fork_awaiter.await_suspend` returned `true`, then `current_coro` is now waiting on the fork.
      // Cancel the await for `current_coro` immediately, as we don't want it to resume automatically.
      // The fork should only resume coroutines that have explicitly called `co_await` on its future.
      fork_awaiter.cancel();
    }
    return current_coro;
  }

  int64_t await_resume() noexcept {
    state = awaitable_impl_::state::end;
    async_stack_watcher_t::await_resume();
    fork_id_watcher_t::await_resume();
    fork_awaiter.await_resume();
    return fork_id;
  }
};

// ================================================================================================

// We can have two distinct awaiters instead: wait_fork_t and wait_fork_result_t.
// The main difference between them is that wait_fork_t can use shared_task::when_ready as an awaiter,
// whereas wait_fork_result_t can use shared_task::operator co_await.
template<typename T>
class wait_fork_t : awaitable_impl_::fork_id_watcher_t, awaitable_impl_::async_stack_watcher_t {
  kphp::coro::shared_task<T> fork_task;
  std::remove_cvref_t<decltype(std::declval<kphp::coro::shared_task<T>>().operator co_await())> fork_awaiter;
  awaitable_impl_::state state{awaitable_impl_::state::init};

  using fork_resume_t = std::remove_cvref_t<decltype(fork_awaiter.await_resume())>;
  using await_resume_t = fork_resume_t;

public:
  explicit wait_fork_t(kphp::coro::shared_task<T> fork_task_) noexcept
      : fork_task(std::move(fork_task_)),
        fork_awaiter(fork_task.operator co_await()) {}

  wait_fork_t(wait_fork_t&& other) noexcept
      : fork_task(std::move(other.fork_task)),
        fork_awaiter(std::move(other.fork_awaiter)),
        state(std::exchange(other.state, awaitable_impl_::state::end)) {}

  wait_fork_t(const wait_fork_t&) = delete;
  wait_fork_t& operator=(const wait_fork_t&) = delete;
  wait_fork_t& operator=(wait_fork_t&&) = delete;

  ~wait_fork_t() {
    if (state == awaitable_impl_::state::suspend) {
      cancel();
    }
  }

  bool await_ready() noexcept {
    kphp::log::assertion(state == awaitable_impl_::state::init);
    state = fork_awaiter.await_ready() ? awaitable_impl_::state::ready : awaitable_impl_::state::init;
    return state == awaitable_impl_::state::ready;
  }

  template<typename promise_t>
  constexpr bool await_suspend(std::coroutine_handle<promise_t> coro) noexcept {
    state = awaitable_impl_::state::suspend;
    return fork_awaiter.await_suspend(coro);
  }

  await_resume_t await_resume() noexcept {
    state = awaitable_impl_::state::end;
    async_stack_watcher_t::await_resume();
    fork_id_watcher_t::await_resume();
    if constexpr (std::is_void_v<await_resume_t>) {
      fork_awaiter.await_resume();
    } else {
      return fork_awaiter.await_resume();
    }
  }

  constexpr bool resumable() const noexcept {
    return state == awaitable_impl_::state::ready || (state == awaitable_impl_::state::suspend && fork_awaiter.resumable());
  }

  constexpr void cancel() noexcept {
    state = awaitable_impl_::state::end;
    fork_awaiter.cancel();
  }
};

// ================================================================================================

template<CancellableAwaitable T>
class wait_with_timeout_t {
  T awaitable;
  wait_for_timer_t timer_awaitable;
  awaitable_impl_::state state{awaitable_impl_::state::init};

  static_assert(CancellableAwaitable<wait_for_timer_t>);
  static_assert(std::is_void_v<decltype(timer_awaitable.await_suspend(std::coroutine_handle<>{}))>);
  using awaitable_suspend_t = decltype(awaitable.await_suspend(std::coroutine_handle<>{}));
  using await_suspend_return_t = awaitable_suspend_t;
  using awaitable_resume_t = decltype(awaitable.await_resume());
  using await_resume_return_t = std::conditional_t<std::is_void_v<awaitable_resume_t>, void, std::optional<awaitable_resume_t>>;

public:
  wait_with_timeout_t(T&& awaitable_, std::chrono::nanoseconds timeout) noexcept
      : awaitable(std::move(awaitable_)),
        timer_awaitable(timeout) {}

  wait_with_timeout_t(wait_with_timeout_t&& other) noexcept
      : awaitable(std::move(other.awaitable)),
        timer_awaitable(std::move(other.timer_awaitable)),
        state(std::exchange(other.state, awaitable_impl_::state::end)) {}

  wait_with_timeout_t(const wait_with_timeout_t&) = delete;
  wait_with_timeout_t& operator=(const wait_with_timeout_t&) = delete;
  wait_with_timeout_t& operator=(wait_with_timeout_t&&) = delete;
  ~wait_with_timeout_t() = default;

  constexpr bool await_ready() noexcept {
    kphp::log::assertion(state == awaitable_impl_::state::init);
    state = awaitable.await_ready() || timer_awaitable.await_ready() ? awaitable_impl_::state::ready : awaitable_impl_::state::init;
    return state == awaitable_impl_::state::ready;
  }

  // according to C++ standard, there can be 3 possible cases:
  // 1. await_suspend returns void;
  // 2. await_suspend returns bool;
  // 3. await_suspend returns std::coroutine_handle<>.
  // we must guarantee that 'co_await wait_with_timeout_t{awaitable, timeout}' behaves like 'co_await awaitable' except
  // it may cancel 'co_await awaitable' if the timeout has elapsed.
  template<typename promise_t>
  await_suspend_return_t await_suspend(std::coroutine_handle<promise_t> coro) noexcept {
    // as we don't rely on coroutine scheduler implementation, let's always suspend awaitable first. in case of some smart scheduler
    // it won't have any effect, but it will have an effect if our scheduler is quite simple.
    state = awaitable_impl_::state::suspend;
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
    state = awaitable_impl_::state::end;
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
wait_with_timeout_t(T&&, std::chrono::nanoseconds) -> wait_with_timeout_t<T>;
