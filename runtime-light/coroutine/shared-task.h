// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

#include "common/containers/intrusive-list.h"
#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/coroutine/control-functions.h"
#include "runtime-light/coroutine/detail/allocator/coroutine-malloc-interface.h"
#include "runtime-light/coroutine/task-allocator-guard.h"
#include "runtime-light/coroutine/void-value.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::coro {

namespace shared_task_impl {

template<typename promise_type>
struct promise_base : public kphp::coro::async_stack_element {
private:
  struct not_started_tag {};
  struct done_tag {};

public:
  constexpr auto initial_suspend() const noexcept -> std::suspend_always {
    return {};
  }

  constexpr auto final_suspend() const noexcept {
    struct awaiter {
      constexpr auto await_ready() const noexcept -> bool {
        return false;
      }

      auto await_suspend(std::coroutine_handle<promise_type> coro) const noexcept -> std::coroutine_handle<> {
        promise_base& promise{coro.promise()};
        vk::intrusive::list<vk::intrusive::list_node<std::coroutine_handle<>>> awaiters;
        awaiters.splice(awaiters.begin(), std::get<vk::intrusive::list<vk::intrusive::list_node<std::coroutine_handle<>>>>(promise.m_state));
        promise.m_state = done_tag{};
        if (awaiters.empty()) {
          return std::noop_coroutine();
        }

        while (true) {
          auto coroutine{awaiters.front()};
          awaiters.pop_front();
          if (awaiters.empty()) {
            // return last awaiter's coroutine_handle to allow it to potentially be compiled as a tail-call
            return coroutine;
          }

          auto& async_stack_root{*promise.get_async_stack_frame().async_stack_root};
          kphp::coro::resume(coroutine, async_stack_root);
        }
      }

      constexpr auto await_resume() const noexcept -> void {}
    };
    return awaiter{};
  }

  auto unhandled_exception() const noexcept -> void {
    kphp::log::error("internal unhandled exception");
  }

  auto done() const noexcept -> bool {
    return std::holds_alternative<done_tag>(m_state);
  }

  auto add_ref() noexcept -> void {
    ++m_refcnt;
  }

  // try to enqueue a awaiter to the list of awaiters.
  //
  // return true if the awaiter was successfully queued, in which case
  // awaiter->coroutine will be resumed when the task completes.
  // false if the coroutine was already completed and the awaiting
  // coroutine can continue without suspending.
  auto suspend_awaiter(vk::intrusive::list_node<std::coroutine_handle<>>& awaiter) noexcept -> bool {
    // NOTE: If the coroutine is not yet started then the first awaiter
    // will start the coroutine before enqueuing itself up to the list
    // of suspended awaiters waiting for completion. We split this into
    // two steps to allow the first awaiter to return without suspending.
    // This avoids recursively resuming the first awaiter inside the call to
    // coroutine.resume() in the case that the coroutine completes
    // synchronously, which could otherwise lead to stack-overflow if
    // the awaiting coroutine awaited many synchronously-completing
    // tasks in a row.

    // start the coroutine if not yet started
    if (std::holds_alternative<not_started_tag>(m_state)) {
      m_state = vk::intrusive::list<vk::intrusive::list_node<std::coroutine_handle<>>>{};
      const auto& handle{std::coroutine_handle<promise_type>::from_promise(*static_cast<promise_type*>(this))};
      auto& async_stack_root{*get_async_stack_frame().async_stack_root};
      kphp::coro::resume(handle, async_stack_root);
    }

    // coroutine already completed, don't suspend
    if (done()) {
      return false;
    }

    std::get<vk::intrusive::list<vk::intrusive::list_node<std::coroutine_handle<>>>>(m_state).push_front(awaiter);

    return true;
  }

  // return true if successfully detached, false if this was the last
  // reference to the coroutine, in which case the caller must
  // call destroy() on the coroutine handle.
  auto detach() noexcept -> bool {
    return m_refcnt-- != 1;
  }

  template<typename... Args>
  auto operator new(size_t n, [[maybe_unused]] Args&&... args) noexcept -> void* {
    return kphp::coro::detail::memory::alloc(n);
  }

  template<typename... Args>
  auto operator new(size_t n, std::align_val_t al, [[maybe_unused]] Args&&... args) noexcept -> void* {
    return kphp::coro::detail::memory::alloc_aligned(n, al);
  }

  auto operator delete(void* ptr, [[maybe_unused]] size_t n) noexcept -> void {
    kphp::coro::detail::memory::free(ptr);
  }

private:
  uint32_t m_refcnt{1};
  std::variant<not_started_tag, done_tag, vk::intrusive::list<vk::intrusive::list_node<std::coroutine_handle<>>>> m_state;
};

template<typename promise_type>
class awaiter_base : private kphp::coro::task_allocator_guard {
  void set_async_top_frame(async_stack_frame& caller_frame, void* return_address) noexcept {
    /**
     * shared_task is the top of the stack for calls from it.
     * Therefore, it's awaiter doesn't store caller_frame, but it save `await_suspend()` return address
     * */
    async_stack_frame& callee_frame{m_coro.promise().get_async_stack_frame()};

    callee_frame.return_address = return_address;
    auto* async_stack_root{caller_frame.async_stack_root};
    kphp::log::assertion(async_stack_root != nullptr);
    callee_frame.async_stack_root = async_stack_root;
    async_stack_root->top_async_stack_frame = std::addressof(callee_frame);
  }

  void reset_async_top_frame(async_stack_frame& caller_frame) noexcept {
    auto* async_stack_root{caller_frame.async_stack_root};
    kphp::log::assertion(async_stack_root != nullptr);
    async_stack_root->top_async_stack_frame = std::addressof(caller_frame);
  }

protected:
  vk::intrusive::list_node<std::coroutine_handle<>> m_awaiting_coroutine_node;
  std::coroutine_handle<promise_type> m_coro;

public:
  explicit awaiter_base(std::coroutine_handle<promise_type> coro) noexcept
      : m_coro(coro) {}

  awaiter_base(awaiter_base&& other) noexcept
      : kphp::coro::task_allocator_guard(std::move(other)),
        m_awaiting_coroutine_node(std::move(other.m_awaiting_coroutine_node)),
        m_coro(std::exchange(other.m_coro, {})) {}

  awaiter_base(const awaiter_base& other) = delete;
  awaiter_base& operator=(const awaiter_base& other) = delete;
  awaiter_base& operator=(awaiter_base&& other) = delete;
  ~awaiter_base() = default;

  auto await_ready() const noexcept -> bool {
    return m_coro.promise().done();
  }

  template<std::derived_from<kphp::coro::async_stack_element> caller_promise_type>
  [[clang::noinline]] auto await_suspend(std::coroutine_handle<caller_promise_type> awaiting_coroutine) noexcept -> bool {
    set_async_top_frame(awaiting_coroutine.promise().get_async_stack_frame(), STACK_RETURN_ADDRESS);
    m_awaiting_coroutine_node.value() = awaiting_coroutine;
    bool suspended{m_coro.promise().suspend_awaiter(m_awaiting_coroutine_node)};
    reset_async_top_frame(awaiting_coroutine.promise().get_async_stack_frame());
    return suspended;
  }

  auto await_resume() noexcept -> void {}
};

} // namespace shared_task_impl

template<typename T = void>
struct shared_task final {
  template<std::same_as<T> F>
  struct promise_non_void;
  struct promise_void;

  using promise_type = std::conditional_t<!std::is_void_v<T>, promise_non_void<T>, promise_void>;

  explicit shared_task(std::coroutine_handle<> coro) noexcept
      : m_haddress(coro.address()) {}

  shared_task(const shared_task& other) noexcept
      : m_haddress(other.m_haddress) {
    if (m_haddress != nullptr) [[likely]] {
      std::coroutine_handle<promise_type>::from_address(m_haddress).promise().add_ref();
    }
  }

  shared_task(shared_task&& other) noexcept
      : m_haddress(std::exchange(other.m_haddress, nullptr)) {}

  shared_task& operator=(const shared_task& other) noexcept {
    if (m_haddress != other.m_haddress) [[likely]] {
      destroy();
      m_haddress = other.m_haddress;
      if (m_haddress) [[likely]] {
        std::coroutine_handle<promise_type>::from_address(m_haddress).promise().add_ref();
      }
    }
    return *this;
  }

  shared_task& operator=(shared_task&& other) noexcept {
    if (this != std::addressof(other)) [[likely]] {
      destroy();
      m_haddress = std::exchange(other.m_haddress, nullptr);
    }
    return *this;
  }

  ~shared_task() {
    if (m_haddress) {
      destroy();
    }
  }

  struct promise_base : public shared_task_impl::promise_base<promise_type> {
    auto get_return_object() noexcept -> shared_task {
      return shared_task{std::coroutine_handle<promise_type>::from_promise(*static_cast<promise_type*>(this))};
    }

    static auto get_return_object_on_allocation_failure() noexcept -> shared_task {
      kphp::log::error("cannot allocate memory for shared_task");
    }
  };

  template<std::same_as<T> F>
  struct promise_non_void final : public promise_base {
    promise_non_void() noexcept = default;
    promise_non_void(const promise_non_void& other) = delete;
    promise_non_void(promise_non_void&& other) = delete;
    promise_non_void& operator=(const promise_non_void& other) = delete;
    promise_non_void& operator=(promise_non_void&& other) = delete;

    ~promise_non_void() {
      std::launder(reinterpret_cast<T*>(m_bytes))->~T();
    }

    template<typename E>
    requires std::constructible_from<F, E&&>
    auto return_value(E&& e) noexcept -> void {
      ::new (m_bytes) F(std::forward<E>(e));
    }

    auto result() const noexcept -> const T& {
      return *std::launder(reinterpret_cast<const T*>(m_bytes));
    }

    alignas(F) std::byte m_bytes[sizeof(F)]{};
  };

  struct promise_void final : public promise_base {
    constexpr auto return_void() const noexcept -> void {}

    constexpr auto result() const noexcept -> void {}
  };

  auto operator co_await() const noexcept {
    using awaiter_base = shared_task_impl::awaiter_base<promise_type>;
    struct awaiter final : public awaiter_base {
      using awaiter_base::awaiter_base;
      auto await_resume() noexcept -> T {
        awaiter_base::await_resume();
        return awaiter_base::m_coro.promise().result();
      }
    };
    return awaiter{std::coroutine_handle<promise_type>::from_address(m_haddress)};
  }

  auto try_get_result() const noexcept {
    const auto& promise{std::coroutine_handle<promise_type>::from_address(m_haddress).promise()};

    if constexpr (std::is_void_v<T>) {
      return promise.done() ? std::optional{kphp::coro::void_value{}} : std::nullopt;
    } else {
      return promise.done() ? std::optional{promise.result()} : std::nullopt;
    }
  }

  auto when_ready() const noexcept {
    using awaiter_base = shared_task_impl::awaiter_base<promise_type>;
    struct awaiter final : public awaiter_base {
      using awaiter_base::awaiter_base;
    };
    return awaiter{std::coroutine_handle<promise_type>::from_address(m_haddress)};
  }

  auto get_handle() const noexcept -> std::coroutine_handle<promise_type> {
    return std::coroutine_handle<promise_type>::from_address(m_haddress);
  }

  // conversion functions
  //
  // erase type
  explicit operator shared_task<>() && noexcept {
    return shared_task<>{std::coroutine_handle<>::from_address(std::exchange(m_haddress, nullptr))};
  }

  explicit operator shared_task<>() & noexcept {
    shared_task<T> task_copy{*this};
    return static_cast<shared_task<>>(std::move(task_copy));
  }

  // restore erased type
  template<typename U>
  requires(std::same_as<void, T>)
  explicit operator shared_task<U>() && noexcept {
    return shared_task<U>{std::coroutine_handle<>::from_address(std::exchange(m_haddress, nullptr))};
  }

  template<typename U>
  requires(std::same_as<void, T>)
  explicit operator shared_task<U>() & noexcept {
    shared_task<T> task_copy{*this};
    return static_cast<shared_task<U>>(std::move(task_copy));
  }

private:
  auto destroy() noexcept -> void {
    if (m_haddress == nullptr) [[unlikely]] {
      return;
    }
    auto coro{std::coroutine_handle<promise_type>::from_address(m_haddress)};
    if (!coro.promise().detach()) {
      kphp::coro::destroy(coro);
    }
  }

  void* m_haddress{};
};

} // namespace kphp::coro
