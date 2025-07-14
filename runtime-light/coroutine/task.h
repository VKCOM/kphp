// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <coroutine>
#include <memory>
#include <type_traits>
#include <utility>

#include "common/containers/final_action.h"
#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/utils/logs.h"

namespace kphp::coro {

namespace task_impl {

template<typename promise_type>
struct promise_base : async_stack_element {
  constexpr auto initial_suspend() const noexcept -> std::suspend_always {
    return {};
  }

  constexpr auto final_suspend() const noexcept {
    struct awaiter {
      constexpr auto await_ready() const noexcept -> bool {
        return false;
      }

      auto await_suspend(std::coroutine_handle<promise_type> coro) const noexcept -> std::coroutine_handle<> {
        if (auto& promise{coro.promise()}; promise.m_next != nullptr) [[likely]] {
          pop_async_stack_frame(promise.get_async_stack_frame());
          return std::coroutine_handle<>::from_address(promise.m_next);
        }
        return std::noop_coroutine();
      }

      constexpr auto await_resume() const noexcept -> void {}

    private:
      void pop_async_stack_frame(async_stack_frame& callee_frame) const noexcept {
        auto* caller_frame{callee_frame.caller_async_stack_frame};
        kphp::log::assertion(caller_frame != nullptr);

        auto* async_stack_root{callee_frame.async_stack_root};
        kphp::log::assertion(async_stack_root != nullptr);

        caller_frame->async_stack_root = async_stack_root;
        async_stack_root->top_async_stack_frame = caller_frame;
      }
    };
    return awaiter{};
  }

  auto unhandled_exception() const noexcept -> void {
    kphp::log::error("internal unhandled exception");
  }

  auto done() const noexcept -> bool {
    return std::coroutine_handle<promise_type>::from_promise(*const_cast<promise_type*>(static_cast<const promise_type*>(this))).done();
  }

  template<typename... Args>
  auto operator new(size_t n, [[maybe_unused]] Args&&... args) noexcept -> void* {
    return kphp::memory::script::alloc(n);
  }

  auto operator delete(void* ptr, [[maybe_unused]] size_t n) noexcept -> void {
    kphp::memory::script::free(ptr);
  }

  void* m_next{};
};

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
  std::coroutine_handle<promise_type> m_coro{};

public:
  explicit awaiter_base(std::coroutine_handle<promise_type> coro) noexcept
      : m_coro(coro) {}

  awaiter_base(awaiter_base&& other) noexcept
      : m_coro(std::exchange(other.m_coro, {})) {}

  awaiter_base(const awaiter_base& other) = delete;
  awaiter_base& operator=(const awaiter_base& other) = delete;
  awaiter_base& operator=(awaiter_base&& other) = delete;

  ~awaiter_base() {
    detach_from_async_stack();
  }

  auto await_ready() noexcept -> bool {
    kphp::log::assertion(!std::exchange(m_started, true)); // to make sure it's not co_awaited more than once
    return false;
  }

  template<typename promise_t>
  [[clang::noinline]] auto await_suspend(std::coroutine_handle<promise_t> coro) noexcept -> std::coroutine_handle<promise_type> {
    push_async_stack_frame(coro.promise().get_async_stack_frame(), STACK_RETURN_ADDRESS);
    m_coro.promise().m_next = coro.address();
    return m_coro;
  }

  constexpr auto await_resume() noexcept -> void {}
};

} // namespace task_impl

/**
 * Please, read following documents before trying to understand what's going on here:
 * 1. C++20 coroutines — https://en.cppreference.com/w/cpp/language/coroutines
 * 2. C++ coroutines: understanding symmetric stransfer — https://lewissbaker.github.io/2020/05/11/understanding_symmetric_transfer
 */
template<typename T = void>
struct task {
  template<std::same_as<T> F>
  struct promise_non_void;
  struct promise_void;

  using promise_type = std::conditional_t<!std::is_void_v<T>, promise_non_void<T>, promise_void>;

  task() noexcept = default;

  explicit task(std::coroutine_handle<> coro) noexcept
      : m_haddress(coro.address()) {}

  task(const task& other) noexcept = delete;

  task(task&& other) noexcept
      : m_haddress(std::exchange(other.m_haddress, nullptr)) {}

  task& operator=(const task& other) noexcept = delete;

  task& operator=(task&& other) noexcept {
    std::swap(m_haddress, other.m_haddress);
    return *this;
  }

  ~task() {
    if (m_haddress) {
      std::coroutine_handle<promise_type>::from_address(m_haddress).destroy();
    }
  }

  struct promise_base : task_impl::promise_base<promise_type> {
    auto get_return_object() noexcept -> task {
      return task{std::coroutine_handle<promise_type>::from_promise(*static_cast<promise_type*>(this))};
    }

    static auto get_return_object_on_allocation_failure() noexcept -> task {
      kphp::log::error("cannot allocate memory for task");
    }
  };

  template<std::same_as<T> F>
  struct promise_non_void final : public promise_base {
    template<typename E>
    requires std::constructible_from<F, E&&>
    auto return_value(E&& e) noexcept -> void {
      ::new (bytes) F(std::forward<E>(e));
    }

    auto result() noexcept -> T {
      auto* t{std::launder(reinterpret_cast<T*>(bytes))};
      const auto finalizer{vk::finally([t] noexcept { t->~T(); })};
      return std::move(*t);
    }

    alignas(F) std::byte bytes[sizeof(F)]{};
  };

  struct promise_void final : public promise_base {
    constexpr auto return_void() const noexcept -> void {}

    constexpr auto result() const noexcept -> void {}
  };

  constexpr auto operator co_await() noexcept {
    using awaiter_base = task_impl::awaiter_base<promise_type>;
    struct awaiter final : public awaiter_base {
      using awaiter_base::awaiter_base;
      auto await_resume() noexcept -> T {
        awaiter_base::await_resume();
        return awaiter_base::m_coro.promise().result();
      }
    };
    return awaiter{std::coroutine_handle<promise_type>::from_address(m_haddress)};
  }

  auto get_handle() noexcept -> std::coroutine_handle<promise_type> {
    return std::coroutine_handle<promise_type>::from_address(m_haddress);
  }

  // conversion functions
  //
  // erase type
  explicit operator task<>() && noexcept {
    return task<>{std::coroutine_handle<>::from_address(std::exchange(m_haddress, nullptr))};
  }
  // restore erased type
  template<typename U>
  requires(std::same_as<void, T>)
  explicit operator task<U>() && noexcept {
    return task<U>{std::coroutine_handle<>::from_address(std::exchange(m_haddress, nullptr))};
  }

private:
  void* m_haddress{};
};

} // namespace kphp::coro
