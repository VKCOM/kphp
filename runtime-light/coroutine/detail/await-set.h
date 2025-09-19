// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <coroutine>

#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-light/coroutine/concepts.h"
#include "runtime-light/coroutine/type-traits.h"
#include "runtime-light/coroutine/void-value.h"

namespace kphp::coro {

template<typename return_type>
class await_set;
} // namespace kphp::coro

namespace kphp::coro::detail::await_set {

template<typename return_type>
class await_set_task;

template<typename return_type, typename promise_type>
class await_set_task_promise_base : public kphp::coro::async_stack_element {
  await_set_task<return_type>* m_self_task{};
  kphp::coro::await_set<return_type>::await_broker* m_await_broker{};

public:
  await_set_task_promise_base() noexcept = default;

  template<typename... Args>
  void* operator new(size_t n, [[maybe_unused]] Args&&... args) noexcept {
    return kphp::memory::script::alloc(n);
  }

  void operator delete(void* ptr, [[maybe_unused]] size_t n) noexcept {
    kphp::memory::script::free(ptr);
  }

  std::suspend_always initial_suspend() const noexcept {
    return {};
  }

  auto final_suspend() const noexcept {
    struct completion_notifier {

      constexpr auto await_ready() const noexcept {
        return false;
      }

      auto await_suspend(std::coroutine_handle<promise_type> coroutine) noexcept -> void {
        auto& promise{coroutine.promise()};
        promise.m_await_broker->publish(*promise.m_self_task);
      }

      constexpr auto await_resume() const noexcept -> void {}
    };
    return completion_notifier{};
  }

  void unhandled_exception() const noexcept {
    kphp::log::error("internal unhandled exception");
  }

  auto start(kphp::coro::await_set<return_type>::await_broker& await_broker, await_set_task<return_type>& self_task) {
    m_await_broker = std::addressof(await_broker);
    m_self_task = std::addressof(self_task);
    std::coroutine_handle<promise_type>::from_promise(*static_cast<promise_type*>(this)).resume();
  }
};

template<typename return_type>
class await_set_task {
  template<std::same_as<return_type> T>
  struct await_set_task_promise_non_void;
  struct await_set_task_promise_void;

public:
  using promise_type = std::conditional_t<std::is_void_v<return_type>, await_set_task_promise_void, await_set_task_promise_non_void<return_type>>;

private:
  std::coroutine_handle<promise_type> m_coroutine;

  struct await_set_task_promise_common : public await_set_task_promise_base<return_type, promise_type> {
    auto get_return_object() noexcept {
      return await_set_task{std::coroutine_handle<promise_type>::from_promise(*static_cast<promise_type*>(this))};
    }

    static await_set_task get_return_object_on_allocation_failure() {
      kphp::log::error("cannot allocate memory for await_set_task");
    }
  };

  template<std::same_as<return_type> T>
  struct await_set_task_promise_non_void : public await_set_task_promise_common {
  private:
    std::optional<T> m_result;

  public:
    auto yield_value(return_type&& return_value) {
      m_result = std::move(return_value);
      return await_set_task_promise_common::final_suspend();
    }

    T result() {
      return std::move(*m_result);
    }

    void return_void() {
      kphp::log::assertion(false);
    }
  };

  struct await_set_task_promise_void : public await_set_task_promise_common {
    auto yield_value() const noexcept {
      return await_set_task_promise_common::final_suspend();
    }

    constexpr auto result() const noexcept -> kphp::coro::void_value {
      return {};
    }

    constexpr void return_void() const noexcept {}
  };

  auto start(kphp::coro::await_set<return_type>::await_broker& coordinator) {
    m_coroutine.promise().start(coordinator, *this);
  }

public:
  template<typename T>
  friend class kphp::coro::await_set;

  template<typename T>
  friend bool operator==(const await_set_task<T>& lhs, const await_set_task<T>& rhs);

  explicit await_set_task(std::coroutine_handle<promise_type> coroutine) noexcept
      : m_coroutine(coroutine) {}

  await_set_task(await_set_task&& other) noexcept
      : m_coroutine(std::exchange(other.m_coroutine, {})) {}

  ~await_set_task() {
    if (m_coroutine != nullptr) {
      m_coroutine.destroy();
    }
  }

  await_set_task(const await_set_task&) = delete;
  await_set_task& operator=(const await_set_task&) = delete;
  await_set_task& operator=(await_set_task&&) = delete;

  auto result() noexcept {
    return m_coroutine.promise().result();
  }
};

template<typename return_type>
bool operator==(const await_set_task<return_type>& lhs, const await_set_task<return_type>& rhs) {
  return lhs.m_coroutine == rhs.m_coroutine;
}

template<typename return_type>
class await_set_awaitable {
public:
  using result_type = std::conditional_t<std::is_void_v<return_type>, kphp::coro::void_value, std::remove_cvref_t<return_type>>;

  kphp::coro::await_set<return_type>::await_broker& m_await_broker;
  std::coroutine_handle<> m_coroutine_handle;

  void notify_awaitable_completed() {
    if (m_coroutine_handle) {
      m_coroutine_handle.resume();
    }
  }

public:
  friend class kphp::coro::await_set<return_type>::await_broker;

  await_set_awaitable(kphp::coro::await_set<return_type>::await_broker& await_broker)
      : m_await_broker(await_broker) {}

  await_set_awaitable(const await_set_awaitable&) = delete;
  await_set_awaitable(await_set_awaitable&&) = delete;

  bool await_ready() {
    return false;
  }

  bool await_suspend(std::coroutine_handle<> coroutine_handle) {
    m_coroutine_handle = coroutine_handle;
    return m_await_broker.try_wait(*this);
  }

  std::optional<result_type> await_resume() {
    return std::move(m_await_broker.try_get_result());
  }
};

//todo think more careful about void case
template<kphp::coro::concepts::coroutine coroutine_type>
auto make_await_set_task(coroutine_type coroutine) -> await_set_task<typename kphp::coro::coroutine_traits<coroutine_type>::return_type> {
  if constexpr (std::is_void_v<typename kphp::coro::coroutine_traits<coroutine_type>::return_type>) {
    co_await std::move(coroutine);
    co_return;
  } else {
    co_yield co_await std::move(coroutine);
  }
}

} // namespace kphp::coro::detail::await_set
