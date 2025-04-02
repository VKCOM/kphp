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
#include <type_traits>
#include <utility>

#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/k2-platform/k2-api.h"

namespace kphp::coro {

namespace shared_task_impl {

struct shared_task_waiter final {
  std::coroutine_handle<> m_continuation;
  shared_task_waiter *m_next{};
  shared_task_waiter *m_prev{};
};

template<typename promise_type>
struct promise_base {
  auto initial_suspend() const noexcept -> std::suspend_always {
    return {};
  }

  auto final_suspend() const noexcept {
    struct awaiter {
      auto await_ready() const noexcept -> bool {
        return false;
      }

      auto await_suspend(std::coroutine_handle<promise_type> coro) const noexcept -> std::coroutine_handle<> {
        promise_base &promise{coro.promise()};
        // mark promise as ready
        auto *waiter{static_cast<shared_task_impl::shared_task_waiter *>(std::exchange(promise.m_waiters, std::addressof(promise)))};
        if (waiter == STARTED_NO_WAITERS_VAL) { // no waiters, so just finish this coroutine
          return std::noop_coroutine();
        }

        while (waiter->m_next != nullptr) {
          // read the m_next pointer before resuming the coroutine
          // since resuming the coroutine may destroy the shared_task_waiter value
          auto *next{waiter->m_next};
          waiter->m_continuation.resume();
          waiter = next;
        }
        // return last waiter's coroutine_handle to allow it to potentially be compiled as a tail-call
        return waiter->m_continuation;
      }

      auto await_resume() const noexcept -> void {}
    };
    return awaiter{};
  }

  auto unhandled_exception() const noexcept -> void {
    php_critical_error("internal unhandled exception");
  }

  auto done() const noexcept -> bool {
    return m_waiters == this;
  }

  auto add_ref() noexcept -> void {
    ++m_refcnt;
  }

  // try to enqueue a waiter to the list of waiters.
  //
  // return true if the waiter was successfully queued, in which case
  // waiter->coroutine will be resumed when the task completes.
  // false if the coroutine was already completed and the awaiting
  // coroutine can continue without suspending.
  auto suspend_awaiter(shared_task_impl::shared_task_waiter &waiter) noexcept -> bool {
    const void *const NOT_STARTED_VAL{std::addressof(this->m_waiters)};

    // NOTE: If the coroutine is not yet started then the first waiter
    // will start the coroutine before enqueuing itself up to the list
    // of suspended waiters waiting for completion. We split this into
    // two steps to allow the first awaiter to return without suspending.
    // This avoids recursively resuming the first waiter inside the call to
    // coroutine.resume() in the case that the coroutine completes
    // synchronously, which could otherwise lead to stack-overflow if
    // the awaiting coroutine awaited many synchronously-completing
    // tasks in a row.

    // start the coroutine if not yet started
    if (m_waiters == NOT_STARTED_VAL) {
      m_waiters = STARTED_NO_WAITERS_VAL;
      std::coroutine_handle<promise_type>::from_promise(*static_cast<promise_type *>(this)).resume();
    }
    // coroutine already completed, don't suspend
    if (done()) {
      return false;
    }

    waiter.m_prev = nullptr;
    waiter.m_next = static_cast<shared_task_impl::shared_task_waiter *>(m_waiters);
    // at this point 'm_waiters' can only be 'STARTED_NO_WAITERS_VAL' or 'other'
    if (m_waiters != STARTED_NO_WAITERS_VAL) {
      static_cast<shared_task_waiter *>(m_waiters)->m_prev = std::addressof(waiter);
    }
    m_waiters = static_cast<void *>(std::addressof(waiter));
    return true;
  }

  // return true if successfully detached, false if this was the last
  // reference to the coroutine, in which case the caller must
  // call destroy() on the coroutine handle.
  auto detach() noexcept -> bool {
    return m_refcnt-- != 1;
  }

  auto cancel_awaiter(const shared_task_impl::shared_task_waiter &waiter) noexcept -> void {
    const void *const NOT_STARTED_VAL{std::addressof(this->m_waiters)};
    if (m_waiters == NOT_STARTED_VAL || m_waiters == STARTED_NO_WAITERS_VAL) [[unlikely]] {
      return;
    }

    const auto *waiter_ptr{std::addressof(waiter)};
    if (m_waiters == waiter_ptr) { // waiter is the head of the list
      m_waiters = waiter_ptr->m_next;
    } else if (waiter_ptr->m_next == nullptr) { // waiter is the last in the list
      waiter_ptr->m_prev->m_next = nullptr;
    } else { // waiter is somewhere in the middle of the list
      waiter_ptr->m_next->m_prev = waiter_ptr->m_prev;
      waiter_ptr->m_prev->m_next = waiter_ptr->m_next;
    }
  }

  template<typename... Args>
  auto operator new(size_t n, [[maybe_unused]] Args &&...args) noexcept -> void * {
    // todo:k2 think about args in new
    // todo:k2 make coroutine allocator
    return k2::alloc(n);
  }

  auto operator delete(void *ptr, [[maybe_unused]] size_t n) noexcept -> void {
    k2::free(ptr);
  }

private:
  static constexpr void *STARTED_NO_WAITERS_VAL = nullptr;

  uint32_t m_refcnt{1};
  // Value is either
  // - nullptr          - indicates started, no waiters
  // - &this->w_waiters - indicates the coroutine is not yet started
  // - this             - indicates value is ready
  // - other            - pointer to head item in linked-list of waiters.
  //                      values are of type 'shared_task_impl_::shared_task_waiter_t'.
  //                      indicates that the coroutine has been started.
  void *m_waiters{std::addressof(m_waiters)};
};

template<typename promise_type>
class awaiter_base {
  enum class state : uint8_t { init, suspend, end };
  state m_state{state::init};

protected:
  std::coroutine_handle<promise_type> m_coro;
  shared_task_impl::shared_task_waiter m_waiter{};

public:
  explicit awaiter_base(std::coroutine_handle<promise_type> coro) noexcept
    : m_coro(coro) {}

  awaiter_base(awaiter_base &&other) noexcept
    : m_state(std::exchange(other.m_state, state::end))
    , m_coro(std::exchange(other.m_coro, {}))
    , m_waiter(std::exchange(other.m_waiter, {})) {}

  awaiter_base(const awaiter_base &other) = delete;
  awaiter_base &operator=(const awaiter_base &other) = delete;
  awaiter_base &operator=(awaiter_base &&other) = delete;

  ~awaiter_base() {
    if (m_state == state::suspend) {
      cancel();
    }
  }

  auto await_ready() const noexcept -> bool {
    php_assert(m_state == state::init && m_coro);
    return m_coro.promise().done();
  }

  auto await_suspend(std::coroutine_handle<> awaiter) noexcept -> bool {
    m_state = state::suspend;
    m_waiter.m_continuation = awaiter;
    return m_coro.promise().suspend_awaiter(m_waiter);
  }

  auto await_resume() noexcept -> void {
    m_state = state::end;
  }

  auto resumable() const noexcept -> bool {
    return m_coro.promise().done();
  }

  auto cancel() noexcept -> void {
    m_state = state::end;
    m_coro.promise().cancel_awaiter(m_waiter);
  }
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

  shared_task(const shared_task &other) noexcept
    : m_haddress(other.m_haddress) {
    if (m_haddress != nullptr) [[likely]] {
      std::coroutine_handle<promise_type>::from_address(m_haddress).promise().add_ref();
    }
  }

  shared_task(shared_task &&other) noexcept
    : m_haddress(std::exchange(other.m_haddress, nullptr)) {}

  shared_task &operator=(const shared_task &other) noexcept {
    if (m_haddress != other.m_haddress) [[likely]] {
      destroy();
      m_haddress = other.m_haddress;
      if (m_haddress) [[likely]] {
        std::coroutine_handle<promise_type>::from_address(m_haddress).promise().add_ref();
      }
    }
    return *this;
  }

  shared_task &operator=(shared_task &&other) noexcept {
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
      return shared_task{std::coroutine_handle<promise_type>::from_promise(*static_cast<promise_type *>(this))};
    }

    static auto get_return_object_on_allocation_failure() noexcept -> shared_task {
      php_critical_error("cannot allocate memory for shared_task");
    }
  };

  template<std::same_as<T> F>
  struct promise_non_void final : public promise_base {
    promise_non_void() noexcept = default;
    promise_non_void(const promise_non_void &other) = delete;
    promise_non_void(promise_non_void &&other) = delete;
    promise_non_void &operator=(const promise_non_void &other) = delete;
    promise_non_void &operator=(promise_non_void &&other) = delete;

    ~promise_non_void() {
      std::launder(reinterpret_cast<T *>(m_bytes))->~T();
    }

    template<typename E>
    requires std::constructible_from<F, E &&> auto return_value(E &&e) noexcept -> void {
      ::new (m_bytes) F(std::forward<E>(e));
    }

    auto result() const noexcept -> const T & {
      return *std::launder(reinterpret_cast<const T *>(m_bytes));
    }

    alignas(F) std::byte m_bytes[sizeof(F)]{};
  };

  struct promise_void final : public promise_base {
    auto return_void() const noexcept -> void {}

    auto result() const noexcept -> void {}
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
  // restore erased type
  template<typename U>
  requires(std::same_as<void, T>) explicit operator shared_task<U>() && noexcept {
    return shared_task<U>{std::coroutine_handle<>::from_address(std::exchange(m_haddress, nullptr))};
  }

private:
  auto destroy() noexcept -> void {
    if (m_haddress == nullptr) [[unlikely]] {
      return;
    }
    auto coro{std::coroutine_handle<promise_type>::from_address(m_haddress)};
    if (!coro.promise().detach()) {
      coro.destroy();
    }
  }

  void *m_haddress{};
};

} // namespace kphp::coro
