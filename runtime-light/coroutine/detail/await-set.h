// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <coroutine>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>

#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/coroutine/await-set-enums.h"
#include "runtime-light/coroutine/concepts.h"
#include "runtime-light/coroutine/type-traits.h"
#include "runtime-light/coroutine/void-value.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::coro::detail::await_set {

struct await_set_waiter {
  await_set_waiter* m_next{};
  await_set_waiter* m_prev{};
  std::coroutine_handle<> m_continuation;
};

template<typename return_type, kphp::coro::await_set_waiting_policy waiting_policy>
class await_set_task;

template<typename return_type, kphp::coro::await_set_waiting_policy waiting_policy>
struct await_set_ready_task_element {
  await_set_ready_task_element* m_next{};
  kphp::stl::list<await_set_task<return_type, waiting_policy>, kphp::memory::script_allocator>::iterator m_storage_location{};
};

template<typename return_type, kphp::coro::await_set_waiting_policy waiting_policy>
class await_broker {
  using await_set_t = kphp::coro::await_set<return_type, waiting_policy>;
  await_set_t& m_await_set;
  await_set_waiter* m_waiters{};
  await_set_ready_task_element<return_type, waiting_policy>* m_ready_tasks{};

public:
  explicit await_broker(await_set_t& await_set) noexcept
      : m_await_set(await_set) {}

  await_broker(const await_broker&) = delete;
  await_broker(await_broker&&) = delete;

  await_broker& operator=(const await_broker&) = delete;
  await_broker& operator=(await_broker&& other) = delete;

  void push_ready_task(await_set_ready_task_element<return_type, waiting_policy>& ready_task) noexcept {
    ready_task.m_next = std::exchange(m_ready_tasks, std::addressof(ready_task));

    if (m_waiters != nullptr) {
      resume_first_awaiter();
    }

    if constexpr (waiting_policy == kphp::coro::await_set_waiting_policy::resume_on_empty) {
      if (m_await_set.empty()) {
        // Release everyone else who was waiting for the task to be completed
        detach_awaiters();
      }
    }
  }

  void cancel_awaiter(await_set_waiter& waiter) noexcept {
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

  bool suspend_awaiter(await_set_waiter& waiter) noexcept {
    if (m_ready_tasks != nullptr) {
      // There are completed tasks
      return false;
    }
    if (m_await_set.m_running_state == await_set_t::running_state_t::stopped) {
      // await_set is shutting down
      return false;
    }
    if constexpr (waiting_policy == kphp::coro::await_set_waiting_policy::resume_on_empty) {
      if (m_await_set.empty()) {
        return false;
      }
    }

    waiter.m_prev = nullptr;
    waiter.m_next = m_waiters;
    if (m_waiters != nullptr) {
      m_waiters->m_prev = std::addressof(waiter);
    }
    m_waiters = std::addressof(waiter);
    return true;
  }

  auto try_get_result() noexcept {
    using result_t = std::expected<decltype(std::declval<await_set_task<return_type, waiting_policy>>().result()), kphp::coro::await_set_next_result_t>;
    if (m_ready_tasks == nullptr) {
      return result_t{std::unexpected{kphp::coro::await_set_next_result_t::empty}};
    }
    if (m_await_set.m_running_state == await_set_t::running_state_t::stopped) {
      return result_t{std::unexpected{kphp::coro::await_set_next_result_t::stopped}};
    }

    auto* ready_task{m_ready_tasks};
    m_ready_tasks = m_ready_tasks->m_next;
    auto task_iterator{ready_task->m_storage_location};
    auto result{task_iterator->result()};
    m_await_set.m_tasks_storage.erase(task_iterator);
    return result_t{std::move(result)};
  }

  void detach_awaiters() noexcept {
    while (m_waiters != nullptr) {
      resume_first_awaiter();
    }
  }

  void abort_all() noexcept {
    detach_awaiters();
    m_ready_tasks = nullptr;
  }

  ~await_broker() {
    abort_all();
  }

private:
  void resume_first_awaiter() noexcept {
    auto* waiter{m_waiters};
    m_waiters = waiter->m_next;
    if (m_waiters != nullptr) {
      m_waiters->m_prev = nullptr;
    }

    kphp::log::assertion(waiter != nullptr);
    waiter->m_continuation.resume();
  }
};

template<typename return_type, kphp::coro::await_set_waiting_policy waiting_policy, typename promise_type>
class await_set_task_promise_base : public kphp::coro::async_stack_element {
  await_broker<return_type, waiting_policy>* m_await_broker{};
  await_set_ready_task_element<return_type, waiting_policy> m_ready_task_element{};

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
        promise.m_await_broker->push_ready_task(promise.m_ready_task_element);
      }

      constexpr auto await_resume() const noexcept -> void {}
    };
    return completion_notifier{};
  }

  void unhandled_exception() const noexcept {
    kphp::log::error("internal unhandled exception");
  }

  auto start(detail::await_set::await_broker<return_type, waiting_policy>& await_broker,
             kphp::stl::list<await_set_task<return_type, waiting_policy>, kphp::memory::script_allocator>::iterator storage_location,
             kphp::coro::async_stack_root& async_stack_root, void* return_address) noexcept {
    m_await_broker = std::addressof(await_broker);
    m_ready_task_element.m_storage_location = storage_location;

    /**
     * initialize await_set_task's async stack frame and make it the top frame.
     * await_set_task is the top of the stack for calls from it.
     * Therefore, it doesn't store caller_frame, but it save `await_set::push()` return address
     * */
    auto& async_stack_frame{get_async_stack_frame()};
    async_stack_frame.caller_async_stack_frame = nullptr;
    async_stack_frame.async_stack_root = std::addressof(async_stack_root);
    async_stack_frame.return_address = return_address;
    async_stack_frame.async_stack_root->top_async_stack_frame = std::addressof(async_stack_frame);

    std::coroutine_handle<promise_type>::from_promise(*static_cast<promise_type*>(this)).resume();
  }
};

template<typename return_type, kphp::coro::await_set_waiting_policy waiting_policy>
class await_set_task {
  template<std::same_as<return_type> T>
  struct await_set_task_promise_non_void;
  struct await_set_task_promise_void;

public:
  using promise_type = std::conditional_t<std::is_void_v<return_type>, await_set_task_promise_void, await_set_task_promise_non_void<return_type>>;

private:
  std::coroutine_handle<promise_type> m_coroutine;

  struct await_set_task_promise_common : public await_set_task_promise_base<return_type, waiting_policy, promise_type> {
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
    auto yield_value(return_type&& return_value) noexcept {
      m_result = std::move(return_value);
      return await_set_task_promise_common::final_suspend();
    }

    T result() noexcept {
      return std::move(*m_result);
    }

    void return_void() noexcept {
      kphp::log::assertion(false);
    }
  };

  struct await_set_task_promise_void : public await_set_task_promise_common {
    constexpr auto result() const noexcept -> kphp::coro::void_value {
      return {};
    }

    constexpr void return_void() const noexcept {}
  };

  auto start(detail::await_set::await_broker<return_type, waiting_policy>& await_broker,
             kphp::stl::list<await_set_task<return_type, waiting_policy>, kphp::memory::script_allocator>::iterator storage_location,
             kphp::coro::async_stack_root& async_stack_root, void* return_address) noexcept {
    m_coroutine.promise().start(await_broker, storage_location, async_stack_root, return_address);
  }

public:
  template<typename T, kphp::coro::await_set_waiting_policy>
  friend class kphp::coro::await_set;

  explicit await_set_task(std::coroutine_handle<promise_type> coroutine) noexcept
      : m_coroutine(coroutine) {}

  await_set_task(await_set_task&& other) noexcept
      : m_coroutine(std::exchange(other.m_coroutine, {})) {}

  await_set_task(const await_set_task&) = delete;
  await_set_task& operator=(const await_set_task&) = delete;
  await_set_task& operator=(await_set_task&&) = delete;

  auto result() noexcept {
    return m_coroutine.promise().result();
  }

  ~await_set_task() {
    if (m_coroutine != nullptr) {
      m_coroutine.destroy();
    }
  }
};

template<typename return_type, kphp::coro::await_set_waiting_policy waiting_policy>
class await_set_awaitable {
public:
  using result_type = std::conditional_t<std::is_void_v<return_type>, kphp::coro::void_value, std::remove_cvref_t<return_type>>;

private:
  await_broker<return_type, waiting_policy>& m_await_broker;

  class awaiter {
    bool m_suspended{};
    await_broker<return_type, waiting_policy>& m_await_broker;
    await_set_waiter m_waiter{};
    kphp::coro::async_stack_frame* caller_frame{};

  public:
    explicit awaiter(await_broker<return_type, waiting_policy>& await_broker) noexcept
        : m_await_broker(await_broker) {}

    awaiter(awaiter&& other) noexcept = delete;
    awaiter(const awaiter& other) = delete;
    awaiter& operator=(const awaiter& other) = delete;
    awaiter& operator=(awaiter&& other) = delete;

    constexpr bool await_ready() noexcept {
      return false;
    }

    template<std::derived_from<kphp::coro::async_stack_element> caller_promise_type>
    bool await_suspend(std::coroutine_handle<caller_promise_type> awaiting_coroutine) noexcept {
      // save caller async stack frame
      caller_frame = std::addressof(awaiting_coroutine.promise().get_async_stack_frame());

      m_waiter.m_continuation = awaiting_coroutine;
      m_suspended = m_await_broker.suspend_awaiter(m_waiter);
      return m_suspended;
    }
    std::expected<result_type, kphp::coro::await_set_next_result_t> await_resume() noexcept {
      // restore caller async stack frame
      kphp::log::assertion(caller_frame != nullptr);
      auto* async_stack_root{caller_frame->async_stack_root};
      kphp::log::assertion(async_stack_root != nullptr);
      async_stack_root->top_async_stack_frame = caller_frame;

      m_suspended = false;
      return m_await_broker.try_get_result();
    }

    ~awaiter() {
      if (m_suspended) {
        m_await_broker.cancel_awaiter(m_waiter);
      }
    }
  };

public:
  explicit await_set_awaitable(await_broker<return_type, waiting_policy>& await_broker) noexcept
      : m_await_broker(await_broker) {}

  await_set_awaitable(const await_set_awaitable&) = delete;
  await_set_awaitable(await_set_awaitable&& other) noexcept
      : m_await_broker(other.m_await_broker) {}

  await_set_awaitable& operator=(const await_set_awaitable&) = delete;
  await_set_awaitable& operator=(await_set_awaitable&& other) = delete;

  auto operator co_await() noexcept {
    return awaiter{m_await_broker};
  }
};

template<kphp::coro::await_set_waiting_policy waiting_policy, kphp::coro::concepts::awaitable awaitable_type>
auto make_await_set_task(awaitable_type coroutine) noexcept
    -> await_set_task<typename kphp::coro::awaitable_traits<awaitable_type>::awaiter_return_type, waiting_policy> {
  if constexpr (std::is_void_v<typename kphp::coro::awaitable_traits<awaitable_type>::awaiter_return_type>) {
    co_await std::move(coroutine);
  } else {
    co_yield co_await std::move(coroutine);
  }
}

} // namespace kphp::coro::detail::await_set
