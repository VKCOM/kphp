// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <concepts>
#include <coroutine>
#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include "runtime-light/coroutine/task.h"
#include "runtime-light/utils/logs.h"

namespace kphp::coro {

struct void_value {};

} // namespace kphp::coro

namespace kphp::coro::detail::when_all {

class when_all_latch {
  size_t m_count{1};
  std::coroutine_handle<> m_awaiting_coroutine;

public:
  explicit when_all_latch(size_t count) noexcept
      : m_count(count + 1) {}

  when_all_latch(when_all_latch&& other) noexcept
      : m_count(std::exchange(other.m_count, 1)),
        m_awaiting_coroutine(std::exchange(other.m_awaiting_coroutine, {})) {}

  when_all_latch& operator=(when_all_latch&& other) noexcept {
    if (this != std::addressof(other)) {
      m_count = std::exchange(other.m_count, 1);
      m_awaiting_coroutine = std::exchange(other.m_awaiting_coroutine, {});
    }
    return *this;
  }

  when_all_latch(const when_all_latch&) = delete;
  when_all_latch& operator=(const when_all_latch&) = delete;

  ~when_all_latch() = default;

  auto is_ready() const noexcept -> bool {
    return m_awaiting_coroutine != nullptr && m_count == 1;
  }

  auto try_await(std::coroutine_handle<> awaiting_coroutine) noexcept -> bool {
    m_awaiting_coroutine = awaiting_coroutine;
    return m_count == 1;
  }

  auto notify_awaitable_completed() noexcept -> void {
    if (--m_count == 1) {
      m_awaiting_coroutine.resume(); // TODO async stack?
    }
  }
};

template<typename task_container_type>
class when_all_ready_awaitable;

template<>
class when_all_ready_awaitable<std::tuple<>> {
public:
  constexpr when_all_ready_awaitable() noexcept = default;
  explicit constexpr when_all_ready_awaitable(std::tuple<> /*unused*/) noexcept {}

  constexpr auto await_ready() const noexcept -> bool {
    return true;
  }

  constexpr auto await_suspend(std::coroutine_handle<> /*unused*/) noexcept -> void {}

  constexpr auto await_resume() const noexcept -> std::tuple<> {
    return {};
  }
};

template<typename... task_types>
class when_all_ready_awaitable<std::tuple<task_types...>> {
  when_all_latch m_latch;
  std::tuple<task_types...> m_tasks;

public:
  explicit when_all_ready_awaitable(task_types&&... tasks) noexcept
      : m_latch(sizeof...(task_types)),
        m_tasks(std::move(tasks)...) {}

  when_all_ready_awaitable(when_all_ready_awaitable&& other) noexcept
      : m_latch(std::move(other.m_latch)),
        m_tasks(std::move(other.m_tasks)) {}

  ~when_all_ready_awaitable() = default;

  when_all_ready_awaitable(const when_all_ready_awaitable&) = delete;
  when_all_ready_awaitable& operator=(const when_all_ready_awaitable&) = delete;
  when_all_ready_awaitable& operator=(when_all_ready_awaitable&&) = delete;

  auto operator co_await() noexcept {
    struct awaiter {
      when_all_ready_awaitable& m_awaitable;

      explicit awaiter(when_all_ready_awaitable& awaitable) noexcept
          : m_awaitable(awaitable) {}

      auto await_ready() const noexcept -> bool {
        return m_awaitable.m_latch.is_ready();
      }

      auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> bool {
        std::apply([&latch = m_awaitable.m_latch](auto&... tasks) noexcept { (tasks.start(latch), ...); }, m_awaitable.m_tasks);
        return m_awaitable.m_latch.try_await(awaiting_coroutine);
      }

      auto await_resume() noexcept {
        return std::apply([this](auto&&... tasks) noexcept { return std::make_tuple(std::move(tasks).return_value()...); }, std::move(m_awaitable.m_tasks));
      }
    };
    return awaiter{*this};
  }
};

template<typename return_type, typename promise_type>
class when_all_task_promise_base {
  when_all_latch* m_latch{};

public:
  when_all_task_promise_base() noexcept = default;

  template<typename... Args>
  auto operator new(size_t n, [[maybe_unused]] Args&&... args) noexcept -> void* {
    return kphp::memory::script::alloc(n);
  }

  auto operator delete(void* ptr, [[maybe_unused]] size_t n) noexcept -> void {
    kphp::memory::script::free(ptr);
  }

  auto initial_suspend() const noexcept -> std::suspend_always {
    return {};
  }

  auto final_suspend() const noexcept {
    struct completion_notifier {
      constexpr auto await_ready() const noexcept {
        return false;
      }

      auto await_suspend(std::coroutine_handle<promise_type> coroutine) noexcept -> void {
        kphp::log::assertion(coroutine.promise().m_latch != nullptr);
        coroutine.promise().m_latch.notify_awaitable_completed();
      }

      constexpr auto await_resume() const noexcept -> void {}
    };
    return completion_notifier{};
  }

  auto unhandled_exception() const noexcept -> void {
    kphp::log::error("internal unhandled exception");
  }

  auto start(when_all_latch& latch) noexcept {
    m_latch = std::addressof(latch);
    std::coroutine_handle<promise_type>::from_promise(*this).resume(); // TODO async stack
  }
};

template<typename return_type>
class when_all_task {
  template<std::same_as<return_type> T>
  struct when_all_task_promise_non_void;
  struct when_all_task_promise_void;

public:
  using when_all_task_promise = std::conditional_t<!std::is_void_v<return_type>, when_all_task_promise_non_void<return_type>, when_all_task_promise_void>;

private:
  std::coroutine_handle<when_all_task_promise> m_coroutine;

  struct when_all_task_promise_common : public when_all_task_promise_base<return_type, when_all_task_promise> {
    auto get_return_object() noexcept {
      return std::coroutine_handle<when_all_task_promise>::from_promise(*this);
    }

    static auto get_return_object_on_allocation_failure() noexcept -> when_all_task {
      kphp::log::error("cannot allocate memory for when_all_task");
    }
  };

  template<std::same_as<return_type> T>
  struct when_all_task_promise_non_void : public when_all_task_promise_common {
    std::add_pointer_t<return_type> m_return_value;

    auto yield_value(return_type&& return_value) noexcept {
      m_return_value = std::addressof(return_value);
      return when_all_task_promise_common::final_suspend();
    }

    template<typename S>
    auto&& result(this S&& self) noexcept {
      return *std::forward<S>(self).m_return_value;
    }

    auto return_void() const noexcept -> void {
      kphp::log::assertion(false); // we should have suspended at co_yield
    }
  };

  struct when_all_task_promise_void : public when_all_task_promise_common {
    constexpr auto result() const noexcept -> void {}
    constexpr auto return_void() const noexcept -> void {}
  };

  auto start(when_all_latch& latch) noexcept -> void {
    m_coroutine.promise().start(latch);
  }

public:
  // to be able to call start()
  template<typename task_container_type>
  friend class when_all_ready_awaitable;

  explicit when_all_task(std::coroutine_handle<when_all_task_promise> coroutine) noexcept
      : m_coroutine(coroutine) {}

  when_all_task(when_all_task&& other) noexcept
      : m_coroutine(std::exchange(other.m_coroutine, {})) {}

  ~when_all_task() {
    if (m_coroutine != nullptr) {
      m_coroutine.destroy();
    }
  }

  when_all_task(const when_all_task&) = delete;
  when_all_task& operator=(const when_all_task&) = delete;
  when_all_task& operator=(when_all_task&&) = delete;

  template<typename S>
  auto&& return_value(this S&& self) noexcept {
    if constexpr (std::is_void_v<return_type>) {
      std::forward<S>(self).m_coroutine.promise().result();
      return kphp::coro::void_value{};
    } else {
      return std::forward<S>(self).m_coroutine.promise().result();
    }
  }
};

template<typename return_type>
inline auto make_when_all_task(kphp::coro::task<return_type> task) noexcept -> when_all_task<return_type> {
  if constexpr (std::is_void_v<return_type>) {
    co_await task;
    co_return;
  } else {
    co_yield co_await task;
  }
}

} // namespace kphp::coro::detail::when_all
