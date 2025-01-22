// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <coroutine>
#include <type_traits>
#include <utility>

#include "common/containers/final_action.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/k2-platform/k2-api.h"

struct task_base_t {
  task_base_t() = default;

  explicit task_base_t(std::coroutine_handle<> handle)
    : handle_address{handle.address()} {}

  task_base_t(task_base_t &&task) noexcept
    : handle_address{std::exchange(task.handle_address, nullptr)} {}

  task_base_t(const task_base_t &task) = delete;

  task_base_t &operator=(task_base_t &&task) noexcept {
    std::swap(handle_address, task.handle_address);
    return *this;
  }

  task_base_t &operator=(const task_base_t &task) = delete;

  ~task_base_t() {
    if (handle_address) {
      std::coroutine_handle<>::from_address(handle_address).destroy();
    }
  }

  bool done() const {
    php_assert(handle_address != nullptr);
    return std::coroutine_handle<>::from_address(handle_address).done();
  }

protected:
  void *handle_address{nullptr};
};

/**
 * Please, read following documents before trying to understand what's going on here:
 * 1. C++20 coroutines — https://en.cppreference.com/w/cpp/language/coroutines
 * 2. C++ coroutines: understanding symmetric stransfer — https://lewissbaker.github.io/2020/05/11/understanding_symmetric_transfer
 */
template<typename T>
struct task_t : public task_base_t {
  template<std::same_as<T> F>
  struct promise_non_void_t;
  struct promise_void_t;

  using promise_type = std::conditional_t<!std::is_void<T>{}, promise_non_void_t<T>, promise_void_t>;
  using task_base_t::task_base_t;

  struct promise_base_t {
    task_t get_return_object() noexcept {
      return task_t{std::coroutine_handle<promise_type>::from_promise(*static_cast<promise_type *>(this))};
    }

    std::suspend_always initial_suspend() {
      return {};
    }

    struct final_suspend_t {
      final_suspend_t() = default;

      bool await_ready() const noexcept {
        return false;
      }

      std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) const noexcept {
        if (h.promise().next) {
          return std::coroutine_handle<>::from_address(h.promise().next);
        }
        return std::noop_coroutine();
      }

      void await_resume() const noexcept {}
    };

    final_suspend_t final_suspend() noexcept {
      return final_suspend_t{};
    }

    void unhandled_exception() {
      exception = std::current_exception();
    }

    void *next = nullptr;
    std::exception_ptr exception;

    static task_t get_return_object_on_allocation_failure() {
      php_critical_error("cannot allocate memory for task_t");
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
  };

  template<std::same_as<T> F>
  struct promise_non_void_t : public promise_base_t {
    template<typename E>
    requires std::constructible_from<F, E &&> void return_value(E &&e) {
      ::new (bytes) F(std::forward<E>(e));
    }

    alignas(F) std::byte bytes[sizeof(F)];
  };

  struct promise_void_t : public promise_base_t {
    void return_void() {}
  };

  T get_result() noexcept {
    if (get_handle().promise().exception) {
      std::rethrow_exception(std::move(get_handle().promise().exception));
    }
    if constexpr (!std::is_void<T>{}) {
      T *t = std::launder(reinterpret_cast<T *>(get_handle().promise().bytes));
      const vk::final_action final_action([t] { t->~T(); });
      return std::move(*t);
    }
  }

  struct awaiter_base_t {
    explicit awaiter_base_t(task_t *task)
      : task{task} {}

    constexpr bool await_ready() const noexcept {
      return false;
    }

    template<typename PromiseType>
    std::coroutine_handle<promise_type> await_suspend(std::coroutine_handle<PromiseType> h) noexcept {
      task->get_handle().promise().next = h.address();
      return task->get_handle();
    }

    bool resumable() const noexcept {
      return task->done();
    }

    void cancel() const noexcept {
      task->get_handle().promise().next = nullptr;
    }

    task_t *task;
  };

  auto operator co_await() {
    struct awaiter_t : public awaiter_base_t {
      using awaiter_base_t::awaiter_base_t;
      T await_resume() noexcept {
        return awaiter_base_t::task->get_result();
      }
    };
    return awaiter_t{this};
  }

  auto when_ready() const noexcept {
    struct awaiter_t : public awaiter_base_t {
      using awaiter_base_t::awaiter_base_t;
      constexpr void await_resume() const noexcept {}
    };
    return awaiter_t{this};
  }

  std::coroutine_handle<promise_type> get_handle() {
    return std::coroutine_handle<promise_type>::from_address(handle_address);
  }

  // conversion functions
  //
  // erase type
  explicit operator task_t<void>() && noexcept {
    return task_t<void>{std::coroutine_handle<>::from_address(std::exchange(handle_address, nullptr))};
  }
  // restore erased type
  template<typename U>
  requires(std::same_as<void, T>) explicit operator task_t<U>() && noexcept {
    return task_t<U>{std::coroutine_handle<>::from_address(std::exchange(handle_address, nullptr))};
  }
};

// === Type traits ================================================================================

template<typename F, typename... Args>
requires std::invocable<F, Args...> inline constexpr bool is_async_function_v = requires {
  {static_cast<task_t<void>>(std::declval<std::invoke_result_t<F, Args...>>())};
};

// ================================================================================================

template<typename F, typename... Args>
requires std::invocable<F, Args...> class async_function_unwrapped_return_type {
  using return_type = std::invoke_result_t<F, Args...>;

  template<typename U>
  struct task_inner {
    using type = U;
  };

  template<typename U>
  struct task_inner<task_t<U>> {
    using type = U;
  };

public:
  using type = task_inner<return_type>::type;
};

template<typename F, typename... Args>
requires std::invocable<F, Args...> using async_function_unwrapped_return_type_t = async_function_unwrapped_return_type<F, Args...>::type;
