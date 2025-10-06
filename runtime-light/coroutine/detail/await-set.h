// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <coroutine>
#include <optional>

#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-light/coroutine/concepts.h"
#include "runtime-light/coroutine/type-traits.h"
#include "runtime-light/coroutine/void-value.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::coro {

template<typename return_type>
class await_set;
} // namespace kphp::coro

namespace kphp::coro::detail::await_set {

template<typename return_type>
class await_set_awaitable;

template<typename return_type>
class await_set_task;

struct await_set_waiter {
  await_set_waiter* m_next{};
  await_set_waiter* m_prev{};
  std::coroutine_handle<> m_continuation;
};

template<typename return_type>
class await_broker {
  kphp::coro::await_set<return_type>& m_await_set;
  await_set_waiter* m_waiters{};
  await_set_task<return_type>* m_ready_tasks{};

public:
  explicit await_broker(kphp::coro::await_set<return_type>& await_set)
      : m_await_set(await_set) {}

  void publish(await_set_task<return_type>& task) {
    if (m_ready_tasks != nullptr) {
      task.m_next = m_ready_tasks;
      m_ready_tasks = std::addressof(task);
    } else {
      m_ready_tasks = std::addressof(task);
    }

    if (m_waiters != nullptr) {
      auto* waiter{m_waiters};
      m_waiters = waiter->m_next;
      if (m_waiters != nullptr) {
        m_waiters->m_prev = nullptr;
      }

      waiter->m_continuation.resume();
    }
  }

  void cancel_awaiter(await_set_waiter& waiter) {
    auto* waiter_ptr{std::addressof(waiter)};
    if (m_waiters == waiter_ptr) {
      m_waiters = waiter_ptr->m_next;
    } else if (waiter_ptr->m_next == nullptr) {
      waiter_ptr->m_prev->m_next = nullptr;
    } else if (m_waiters != nullptr) {
      waiter_ptr->m_prev->m_next = waiter_ptr->m_next;
      waiter_ptr->m_next->m_prev = waiter_ptr->m_prev;
    }
  }

  bool suspend_awaiter(await_set_waiter& waiter) {
    if (m_ready_tasks != nullptr || m_await_set.empty()) {
      return false;
    }

    waiter.m_prev = nullptr;
    waiter.m_next = m_waiters;
    if (m_waiters != nullptr) {
      m_waiters->m_prev = std::addressof(waiter);
    }
    m_waiters = std::addressof(waiter);
    return true;
  }

  std::optional<typename await_set_awaitable<return_type>::result_type> try_get_result() {
    if (m_ready_tasks == nullptr) {
      return std::nullopt;
    }

    auto* ready_task{m_ready_tasks};
    m_ready_tasks = m_ready_tasks->m_next;
    auto result{std::move(ready_task->result())};
    m_await_set.m_tasks_storage.erase(ready_task->m_coroutine.promise().m_storage_location);
    return result;
  }

  void detach_awaiters() {
    while (m_waiters != nullptr) {
      auto* waiter{m_waiters};
      m_waiters = waiter->m_next;
      if (m_waiters != nullptr) {
        m_waiters->m_prev = nullptr;
      }
      waiter->m_continuation.resume();
    }
  }

  ~await_broker() {
    detach_awaiters();
  }
};

template<typename return_type, typename promise_type>
class await_set_task_promise_base : public kphp::coro::async_stack_element {
  detail::await_set::await_broker<return_type>* m_await_broker{};
  std::list<await_set_task<return_type>>::iterator m_storage_location{};

public:
  template<typename T>
  friend class await_broker;

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
        promise.m_await_broker->publish(*promise.m_storage_location);
      }

      constexpr auto await_resume() const noexcept -> void {}
    };
    return completion_notifier{};
  }

  void unhandled_exception() const noexcept {
    kphp::log::error("internal unhandled exception");
  }

  auto start(detail::await_set::await_broker<return_type>& await_broker, std::list<await_set_task<return_type>>::iterator storage_location,
             kphp::coro::async_stack_root& async_stack_root, void* return_address) {
    m_await_broker = std::addressof(await_broker);
    m_storage_location = storage_location;

    auto& async_stack_frame{get_async_stack_frame()};
    // initialize when_all_task's async stack frame and make it the top frame
    async_stack_frame.caller_async_stack_frame = nullptr;
    async_stack_frame.async_stack_root = std::addressof(async_stack_root);
    async_stack_frame.return_address = return_address;
    async_stack_frame.async_stack_root->top_async_stack_frame = std::addressof(async_stack_frame);

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
  await_set_task* m_next{};
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

  auto start(detail::await_set::await_broker<return_type>& await_broker, std::list<await_set_task<return_type>>::iterator storage_location,
             kphp::coro::async_stack_root& async_stack_root, void* return_address) {
    m_coroutine.promise().start(await_broker, storage_location, async_stack_root, return_address);
  }

public:
  template<typename T>
  friend class detail::await_set::await_broker;

  template<typename T>
  friend class kphp::coro::await_set;

  template<typename T>
  friend bool operator==(const await_set_task<T>& lhs, const await_set_task<T>& rhs);

  explicit await_set_task(std::coroutine_handle<promise_type> coroutine) noexcept
      : m_coroutine(coroutine) {}

  await_set_task(await_set_task&& other) noexcept // remove it?
      : m_coroutine(std::exchange(other.m_coroutine, {})) {}

  ~await_set_task() {
    if (m_coroutine != nullptr) {
      m_coroutine.destroy();
    }
  }

  await_set_task(const await_set_task&) = delete;
  await_set_task& operator=(const await_set_task&) = delete;
  await_set_task& operator=(await_set_task&&) = delete;

  auto result() noexcept { // only for &&?
    return m_coroutine.promise().result();
  }
};

template<typename return_type>
class await_set_awaitable {
public:
  using result_type = std::conditional_t<std::is_void_v<return_type>, kphp::coro::void_value, std::remove_cvref_t<return_type>>;

private:
  await_broker<return_type>& m_await_broker;

  class awaiter {
    bool m_suspended{};
    await_broker<return_type>& m_await_broker;
    await_set_waiter m_waiter{};

  public:
    explicit awaiter(await_broker<return_type>& await_broker) noexcept
        : m_await_broker(await_broker) {}

    awaiter(awaiter&& other) noexcept = delete;
    awaiter(const awaiter& other) = delete;
    awaiter& operator=(const awaiter& other) = delete;
    awaiter& operator=(awaiter&& other) = delete;

    bool await_ready() noexcept {
      return false;
    }

    bool await_suspend(std::coroutine_handle<> coroutine_handle) noexcept {
      m_waiter.m_continuation = coroutine_handle;
      m_suspended = m_await_broker.suspend_awaiter(m_waiter);
      return m_suspended;
    }

    std::optional<result_type> await_resume() noexcept {
      m_suspended = false;
      return std::move(m_await_broker.try_get_result());
    }

    ~awaiter() {
      if (m_suspended) {
        m_await_broker.cancel_awaiter(m_waiter);
      }
    }
  };

public:
  await_set_awaitable(await_broker<return_type>& await_broker)
      : m_await_broker(await_broker) {}

  await_set_awaitable(await_set_awaitable&& other)
      : m_await_broker(other.m_await_broker) {}

  auto operator co_await() const noexcept {
    return awaiter{m_await_broker};
  }
};

template<kphp::coro::concepts::awaitable awaitable_type>
auto make_await_set_task(awaitable_type coroutine) -> await_set_task<typename kphp::coro::awaitable_traits<awaitable_type>::awaiter_return_type> {
  if constexpr (std::is_void_v<typename kphp::coro::awaitable_traits<awaitable_type>::awaiter_return_type>) {
    co_await std::move(coroutine);
    co_return;
  } else {
    co_yield co_await std::move(coroutine);
  }
}

} // namespace kphp::coro::detail::await_set
