// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <coroutine>
#include <memory>
#include <utility>

#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/coroutine/detail/allocator/task-allocator.h"
#include "runtime-light/coroutine/task-allocator-guard.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::coro {

namespace detail {

template<typename promise_type>
class awaiter_base {
  void push_async_stack_frame(async_stack_frame& caller_frame, void* return_address) noexcept {
    async_stack_frame& callee_frame{m_coro.promise().get_async_stack_frame()};
    callee_frame.caller_async_stack_frame = std::addressof(caller_frame);
    callee_frame.return_address = return_address;

    auto* async_stack_root{caller_frame.async_stack_root};
    kphp::log::assertion(async_stack_root != nullptr);
    callee_frame.async_stack_root = async_stack_root;
    async_stack_root->top_async_stack_frame = std::addressof(callee_frame);
  }

  void detach_from_async_stack() noexcept {
    async_stack_frame& callee_frame{m_coro.promise().get_async_stack_frame()};
    callee_frame.caller_async_stack_frame = nullptr;
  }

protected:
  bool m_started{};
  bool m_suspended{};
  std::coroutine_handle<promise_type> m_coro{};

public:
  explicit awaiter_base(std::coroutine_handle<promise_type> coro) noexcept
      : m_coro(coro) {}

  awaiter_base(awaiter_base&& other) noexcept
      : m_started(other.m_started),
        m_suspended(std::exchange(other.m_suspended, false)),
        m_coro(std::exchange(other.m_coro, {})) {}

  awaiter_base(const awaiter_base& other) = delete;
  awaiter_base& operator=(const awaiter_base& other) = delete;
  awaiter_base& operator=(awaiter_base&& other) = delete;

  ~awaiter_base() {
    if (m_coro != nullptr && m_suspended) {
      m_coro.promise().m_next = nullptr;
      detach_from_async_stack();
    }
  }

  auto await_ready() noexcept -> bool {
    kphp::log::assertion(!std::exchange(m_started, true)); // to make sure it's not co_awaited more than once
    return false;
  }

  template<std::derived_from<kphp::coro::async_stack_element> caller_promise_type>
  [[clang::noinline]] auto await_suspend(std::coroutine_handle<caller_promise_type> awaiting_coroutine) noexcept -> std::coroutine_handle<promise_type> {
    push_async_stack_frame(awaiting_coroutine.promise().get_async_stack_frame(), STACK_RETURN_ADDRESS);
    m_coro.promise().m_next = awaiting_coroutine.address();
    m_suspended = true;
    return m_coro;
  }

  auto await_resume() noexcept -> void {
    m_suspended = false;
  }
};

template<typename T>
auto make_fork_task(kphp::coro::task<T>&& task) noexcept {
  return kphp::coro::detached_task<T>{std::move(task)};
}

} // namespace detail

template<typename T = void>
struct detached_task {
private:
  using task_promise_type = kphp::coro::task<T>::promise_type;

  explicit detached_task(kphp::coro::task<T>&& task) noexcept
      : m_coro(std::exchange(task.m_coro, {})) {}

public:
  detached_task() noexcept = default;

  detached_task(const detached_task& other) noexcept = delete;

  detached_task(detached_task&& other) noexcept
      : m_coro(std::exchange(other.m_coro, {})),
        m_stack(std::exchange(other.m_stack, nullptr)) {}

  detached_task& operator=(const detached_task& other) noexcept = delete;

  detached_task& operator=(detached_task&& other) noexcept {
    std::swap(m_coro, other.m_coro);
    std::swap(m_stack, other.m_stack);

    return *this;
  }

  ~detached_task() {
    if (m_coro) {
      auto* prev_stack{m_task_allocator.exchange_stack(m_stack)};
      m_coro.destroy();
      m_task_allocator.set_stack(prev_stack);
    }
  }

  constexpr auto operator co_await() noexcept {
    using awaiter_base = detail::awaiter_base<task_promise_type>;
    struct awaiter final : public awaiter_base, private kphp::coro::task_allocator_guard {
      awaiter(std::coroutine_handle<task_promise_type> coro, kphp::coro::detail::memory::task_allocator& task_allocator,
              memory_resource::segmented_stack_resource<kphp::coro::detail::memory::task_allocator::shared_chunk_pool>* stack) noexcept
          : awaiter_base(coro),
            kphp::coro::task_allocator_guard(task_allocator, stack) {}

      auto await_resume() noexcept -> T {
        awaiter_base::await_resume();
        return awaiter_base::m_coro.promise().result();
      }
    };

    return awaiter{m_coro, m_task_allocator, m_stack};
  }

  auto get_handle() noexcept -> std::coroutine_handle<task_promise_type> {
    return m_coro;
  }

  // conversion functions
  //
  // erase type
  explicit operator detached_task<>() && noexcept {
    return detached_task<>{std::exchange(m_coro, {})};
  }

  // restore erased type
  template<typename U>
  requires(std::same_as<void, T>)
  explicit operator detached_task<U>() && noexcept {
    return detached_task<U>{std::exchange(m_coro, {})};
  }

private:
  std::coroutine_handle<task_promise_type> m_coro;
  kphp::coro::detail::memory::task_allocator& m_task_allocator{kphp::coro::detail::memory::task_allocator::get()};
  memory_resource::segmented_stack_resource<kphp::coro::detail::memory::task_allocator::shared_chunk_pool>* m_stack{nullptr};

  template<typename U>
  friend auto detail::make_fork_task(kphp::coro::task<U>&& task) noexcept;
};

} // namespace kphp::coro

#define DETACH_TASK(...) (kphp::coro::task_allocator_guard{}, kphp::coro::detail::make_fork_task(__VA_ARGS__))
