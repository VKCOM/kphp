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

namespace shared_task_impl_ {

struct shared_task_waiter_t final {
  std::coroutine_handle<> m_continuation;
  shared_task_waiter_t *m_next{};
};

template<typename promise_type>
struct promise_base_t {
  constexpr std::suspend_always initial_suspend() const noexcept {
    return {};
  }

  constexpr auto final_suspend() const noexcept {
    struct final_suspend_t {
      constexpr final_suspend_t() noexcept = default;

      constexpr bool await_ready() const noexcept {
        return false;
      }

      constexpr std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> coro) const noexcept {
        promise_base_t &promise{coro.promise()};
        // mark promise as ready
        auto *waiter{static_cast<shared_task_impl_::shared_task_waiter_t *>(std::exchange(promise.m_waiters, std::addressof(promise)))};
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

      constexpr void await_resume() const noexcept {}
    };
    return final_suspend_t{};
  }

  constexpr void unhandled_exception() noexcept {
    php_critical_error("internal unhandled exception");
  }

  constexpr bool ready() const noexcept {
    return m_waiters == this;
  }

  constexpr void add_ref() noexcept {
    ++m_refcnt;
  }

  // try to enqueue a waiter to the list of waiters.
  //
  // return true if the waiter was successfully queued, in which case
  // waiter->coroutine will be resumed when the task completes.
  // false if the coroutine was already completed and the awaiting
  // coroutine can continue without suspending.
  bool suspend(shared_task_impl_::shared_task_waiter_t &waiter, std::coroutine_handle<> coro) noexcept {
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
      coro.resume();
    }
    // coroutine already completed, don't suspend
    if (ready()) {
      return false;
    }

    waiter.m_next = static_cast<shared_task_impl_::shared_task_waiter_t *>(m_waiters);
    m_waiters = static_cast<void *>(std::addressof(waiter));
    return true;
  }

  // return true if successfully detached, false if this was the last
  // reference to the coroutine, in which case the caller must
  // call destroy() on the coroutine handle.
  constexpr bool detach() noexcept {
    return m_refcnt-- != 1;
  }

  void cancel(const shared_task_impl_::shared_task_waiter_t &waiter) noexcept {
    const void *const READY_VAL{this};
    if (m_waiters == READY_VAL) [[unlikely]] {
      php_critical_error("currently, shared_task_t does not support cancellation after it has finished");
    }

    const void *const NOT_STARTED_VAL{std::addressof(this->m_waiters)};
    if (m_waiters == NOT_STARTED_VAL || m_waiters == STARTED_NO_WAITERS_VAL) [[unlikely]] {
      return;
    }

    const auto *waiter_ptr{std::addressof(waiter)};
    if (m_waiters == waiter_ptr) { // waiter is the head of the list
      m_waiters = waiter_ptr->m_next;
      return;
    }

    auto *tmp_waiters{static_cast<shared_task_impl_::shared_task_waiter_t *>(m_waiters)};
    while (tmp_waiters->m_next != nullptr && tmp_waiters->m_next != waiter_ptr) {
      tmp_waiters = tmp_waiters->m_next;
    }
    if (tmp_waiters->m_next == nullptr) [[unlikely]] { // for some reason there is no such waiter
      return;
    }

    tmp_waiters->m_next = tmp_waiters->m_next->m_next;
  }

  template<typename... Args>
  void *operator new(std::size_t n, [[maybe_unused]] Args &&...args) noexcept {
    // todo:k2 think about args in new
    // todo:k2 make coroutine allocator
    void *buffer = k2::alloc(n);
    return buffer;
  }

  void operator delete(void *ptr, [[maybe_unused]] size_t n) noexcept {
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
class awaiter_base_t {
  enum class state : uint8_t { init, suspend, end };
  state m_state{state::init};

protected:
  std::coroutine_handle<promise_type> m_coro;
  shared_task_impl_::shared_task_waiter_t m_waiter{};

public:
  constexpr explicit awaiter_base_t(std::coroutine_handle<promise_type> coro) noexcept
    : m_coro(coro) {}

  awaiter_base_t(awaiter_base_t &&other) noexcept
    : m_state(std::exchange(other.m_state, state::end))
    , m_coro(std::exchange(other.m_coro, {}))
    , m_waiter(std::exchange(other.m_waiter, {})) {}

  awaiter_base_t(const awaiter_base_t &other) = delete;
  awaiter_base_t &operator=(const awaiter_base_t &other) = delete;
  awaiter_base_t &operator=(awaiter_base_t &&other) = delete;

  ~awaiter_base_t() {
    if (m_state == state::suspend) {
      cancel();
    }
  }

  constexpr bool await_ready() const noexcept {
    php_assert(m_state == state::init && m_coro);
    return m_coro.promise().ready();
  }

  constexpr bool await_suspend(std::coroutine_handle<> awaiter) noexcept {
    m_state = state::suspend;
    m_waiter.m_continuation = awaiter;
    return m_coro.promise().suspend(m_waiter, m_coro);
  }

  constexpr void await_resume() noexcept {
    m_state = state::end;
  }

  constexpr bool resumable() const noexcept {
    return m_coro.promise().ready();
  }

  constexpr void cancel() noexcept {
    m_state = state::end;
    m_coro.promise().cancel(m_waiter);
  }
};

} // namespace shared_task_impl_

template<typename T>
struct shared_task_t final {
  template<std::same_as<T> F>
  struct promise_non_void_t;
  struct promise_void_t;

  using promise_type = std::conditional_t<!std::is_void_v<T>, promise_non_void_t<T>, promise_void_t>;

  constexpr explicit shared_task_t(std::coroutine_handle<> coro) noexcept
    : m_haddress(coro.address()) {}

  constexpr shared_task_t(const shared_task_t &other) noexcept
    : m_haddress(other.m_haddress) {
    if (m_haddress != nullptr) [[likely]] {
      std::coroutine_handle<promise_type>::from_address(m_haddress).promise().add_ref();
    }
  }

  constexpr shared_task_t(shared_task_t &&other) noexcept
    : m_haddress(std::exchange(other.m_haddress, nullptr)) {}

  shared_task_t &operator=(const shared_task_t &other) noexcept {
    if (m_haddress != other.m_haddress) [[likely]] {
      destroy();
      m_haddress = other.m_haddress;
      if (m_haddress) [[likely]] {
        std::coroutine_handle<promise_type>::from_address(m_haddress).promise().add_ref();
      }
    }
    return *this;
  }

  shared_task_t &operator=(shared_task_t &&other) noexcept {
    if (this != std::addressof(other)) [[likely]] {
      destroy();
      m_haddress = std::exchange(other.m_haddress, nullptr);
    }
    return *this;
  }

  ~shared_task_t() {
    if (m_haddress) {
      destroy();
    }
  }

  struct promise_base_t : public shared_task_impl_::promise_base_t<promise_type> {
    constexpr shared_task_t get_return_object() noexcept {
      return shared_task_t{std::coroutine_handle<promise_type>::from_promise(*static_cast<promise_type *>(this))};
    }

    static shared_task_t get_return_object_on_allocation_failure() noexcept {
      php_critical_error("cannot allocate memory for shared_task_t");
    }
  };

  template<std::same_as<T> F>
  struct promise_non_void_t final : public promise_base_t {
    constexpr promise_non_void_t() noexcept = default;
    promise_non_void_t(const promise_non_void_t &other) = delete;
    promise_non_void_t(promise_non_void_t &&other) = delete;
    promise_non_void_t &operator=(const promise_non_void_t &other) = delete;
    promise_non_void_t &operator=(promise_non_void_t &&other) = delete;

    ~promise_non_void_t() {
      std::launder(reinterpret_cast<T *>(m_bytes))->~T();
    }

    template<typename E>
    requires std::constructible_from<F, E &&> constexpr void return_value(E &&e) noexcept {
      ::new (m_bytes) F(std::forward<E>(e));
    }

    constexpr const T &result() const noexcept {
      return *std::launder(reinterpret_cast<const T *>(m_bytes));
    }

    alignas(F) std::byte m_bytes[sizeof(F)]{};
  };

  struct promise_void_t final : public promise_base_t {
    constexpr void return_void() const noexcept {}

    constexpr void result() const noexcept {}
  };

  constexpr auto operator co_await() noexcept {
    using awaiter_base_t = shared_task_impl_::awaiter_base_t<promise_type>;
    struct awaiter_t final : public awaiter_base_t {
      using awaiter_base_t::awaiter_base_t;
      constexpr T await_resume() noexcept {
        awaiter_base_t::await_resume();
        return awaiter_base_t::m_coro.promise().result();
      }
    };
    return awaiter_t{std::coroutine_handle<promise_type>::from_address(m_haddress)};
  }

  constexpr auto when_ready() const noexcept {
    using awaiter_base_t = shared_task_impl_::awaiter_base_t<promise_type>;
    struct awaiter_t final : public awaiter_base_t {
      using awaiter_base_t::awaiter_base_t;
    };
    return awaiter_t{std::coroutine_handle<promise_type>::from_address(m_haddress)};
  }

  constexpr std::coroutine_handle<promise_type> get_handle() const noexcept {
    return std::coroutine_handle<promise_type>::from_address(m_haddress);
  }

  // conversion functions
  //
  // erase type
  explicit operator shared_task_t<void>() && noexcept {
    return shared_task_t<void>{std::coroutine_handle<>::from_address(std::exchange(m_haddress, nullptr))};
  }
  // restore erased type
  template<typename U>
  requires(std::same_as<void, T>) explicit operator shared_task_t<U>() && noexcept {
    return shared_task_t<U>{std::coroutine_handle<>::from_address(std::exchange(m_haddress, nullptr))};
  }

private:
  constexpr void destroy() noexcept {
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
