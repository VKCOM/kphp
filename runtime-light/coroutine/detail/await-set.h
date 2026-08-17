// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <coroutine>
#include <cstddef>
#include <expected>
#include <memory>
#include <optional>

#include "common/containers/intrusive-list.h"
#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/coroutine/coroutine-state.h"
#include "runtime-light/coroutine/type-traits.h"
#include "runtime-light/coroutine/void-value.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::coro {

template<typename return_type>
class await_set;

} // namespace kphp::coro

namespace kphp::coro::detail::await_set {

template<typename return_type>
class await_set_task;

template<typename return_type>
struct await_set_ready_task_element {
  await_set_ready_task_element* m_next{};
  vk::intrusive::list<vk::intrusive::list_node<std::coroutine_handle<>>>::iterator m_storage_location;
};

template<typename return_type>
class await_broker {
  vk::intrusive::list<vk::intrusive::list_node<std::coroutine_handle<>>> m_tasks_storage;
  vk::intrusive::list<vk::intrusive::list_node<std::coroutine_handle<>>> m_awaiters;
  await_set_ready_task_element<return_type>* m_ready_tasks{};
  size_t m_tasks_count{0};

public:
  await_broker() noexcept = default;

  await_broker(const await_broker&) = delete;
  await_broker(await_broker&&) = delete;

  await_broker& operator=(const await_broker&) = delete;
  await_broker& operator=(await_broker&& other) = delete;

  template<typename... Args>
  void* operator new(size_t n, [[maybe_unused]] Args&&... args) noexcept {
    return kphp::memory::script::alloc(n);
  }

  template<typename... Args>
  auto operator new(size_t n, std::align_val_t al, [[maybe_unused]] Args&&... args) noexcept -> void* {
    return kphp::memory::script::alloc_aligned(n, al);
  }

  void operator delete(void* ptr, [[maybe_unused]] size_t n) noexcept {
    kphp::memory::script::free(ptr);
  }

  void start_task(await_set_task<return_type>&& task, kphp::coro::async_stack_root& coroutine_stack_root, void* return_address) noexcept {
    auto& promise{task.m_promise};
    m_tasks_storage.push_front(promise.m_coroutine_node);
    ++m_tasks_count;
    auto& instance_state{kphp::coro::instance_state::get()};
    auto* const prev_chain_stats{instance_state.current_chain_stats};
    promise.start(*this, m_tasks_storage.begin(), coroutine_stack_root, return_address);
    instance_state.current_chain_stats = prev_chain_stats;
  }

  void push_ready_task(await_set_ready_task_element<return_type>& ready_task) noexcept {
    ready_task.m_next = std::exchange(m_ready_tasks, std::addressof(ready_task));
    if (!m_awaiters.empty()) {
      auto coroutine{m_awaiters.front()};
      /*
       * We can remove pop_front() here, because list node will be destroyed after resume and in its
       * destructor will call unlink() [1]. But we left pop_front() for better readability and safety
       * (in the future invariant [1] may not work).
       */
      m_awaiters.pop_front();
      coroutine.resume();
    }
  }

  bool suspend_awaiter(vk::intrusive::list_node<std::coroutine_handle<>>& awaiter) noexcept {
    if (m_ready_tasks != nullptr) {
      // There are completed tasks
      return false;
    }

    m_awaiters.push_front(awaiter);

    return true;
  }

  auto try_get_result() noexcept {
    using result_t = std::optional<decltype(std::declval<typename await_set_task<return_type>::promise_type>().result())>;
    if (m_ready_tasks == nullptr) {
      return result_t{std::nullopt};
    }

    auto* ready_task{std::exchange(m_ready_tasks, m_ready_tasks->m_next)};
    auto task_iterator{ready_task->m_storage_location};

    auto typed_handle{std::coroutine_handle<typename await_set_task<return_type>::promise_type>::from_address(task_iterator->address())};
    auto result{typed_handle.promise().result()};

    /*
     * We can remove erase() here, because list node will be destroyed after destruction of coroutine frame and in its
     * destructor will call unlink() [1]. But we left erase() for better readability and safety
     * (in the future invariant [1] may not work).
     */
    m_tasks_storage.erase(task_iterator);
    --m_tasks_count;

    auto& instance_state{kphp::coro::instance_state::get()};
    auto* const prev_chain_stats{instance_state.current_chain_stats};
    instance_state.current_chain_stats = std::addressof(typed_handle.promise().chain_stats());
    const auto finished_chain_stats{typed_handle.promise().chain_stats()};
    typed_handle.destroy();
    instance_state.current_chain_stats = prev_chain_stats;
    instance_state.note_chain_finished(finished_chain_stats);

    return result_t{std::move(result)};
  }

  void abort_all() noexcept {
    m_ready_tasks = nullptr;
    detach_all();
    while (!m_tasks_storage.empty()) {
      auto coroutine{m_tasks_storage.front()};
      /*
       * We can remove pop_front() here, because list node will be destroyed after destruction of coroutine frame and in its
       * destructor will call unlink() [1]. But we left pop_front() for better readability and safety
       * (in the future invariant [1] may not work).
       */
      m_tasks_storage.pop_front();

      auto typed_handle{std::coroutine_handle<typename await_set_task<return_type>::promise_type>::from_address(coroutine.address())};
      auto& instance_state{kphp::coro::instance_state::get()};
      auto* const prev_chain_stats{instance_state.current_chain_stats};
      instance_state.current_chain_stats = std::addressof(typed_handle.promise().chain_stats());
      const auto finished_chain_stats{typed_handle.promise().chain_stats()};
      typed_handle.destroy();
      instance_state.current_chain_stats = prev_chain_stats;
      instance_state.note_chain_finished(finished_chain_stats);
    }

    m_tasks_count = 0;
  }

  void detach_all() noexcept {
    // Extract the value from m_awaiters so as not to resume those who subscribe during the detach_all.
    vk::intrusive::list<vk::intrusive::list_node<std::coroutine_handle<>>> awaiters;
    awaiters.splice(awaiters.begin(), m_awaiters);
    while (!awaiters.empty()) {
      auto coroutine{awaiters.front()};
      /*
       * We can remove pop_front() here, because list node will be destroyed after resume and in its
       * destructor will call unlink() [1]. But we left pop_front() for better readability and safety
       * (in the future invariant [1] may not work).
       */
      awaiters.pop_front();
      coroutine.resume();
    }
  }

  size_t size() noexcept {
    return m_tasks_count;
  }

  ~await_broker() {
    abort_all();
  }

private:
};

template<typename return_type, typename promise_type>
class await_set_task_promise_base : public kphp::coro::async_stack_element {
  std::optional<std::reference_wrapper<await_broker<return_type>>> m_await_broker{};
  await_set_ready_task_element<return_type> m_ready_task_element{};
  kphp::coro::chain_stats m_chain_stats{};

public:
  await_set_task_promise_base() noexcept = default;

  kphp::coro::chain_stats& chain_stats() noexcept {
    return m_chain_stats;
  }

  template<typename... Args>
  void* operator new(size_t n, [[maybe_unused]] Args&&... args) noexcept {
    return kphp::memory::script::alloc(n);
  }

  template<typename... Args>
  auto operator new(size_t n, std::align_val_t al, [[maybe_unused]] Args&&... args) noexcept -> void* {
    return kphp::memory::script::alloc_aligned(n, al);
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
        auto opt_await_broker{promise.m_await_broker};
        kphp::log::assertion(opt_await_broker.has_value());
        (*opt_await_broker).get().push_ready_task(promise.m_ready_task_element);
      }

      constexpr auto await_resume() const noexcept -> void {}
    };
    return completion_notifier{};
  }

  void unhandled_exception() const noexcept {
    kphp::log::error("internal unhandled exception");
  }

  auto start(detail::await_set::await_broker<return_type>& await_broker,
             vk::intrusive::list<vk::intrusive::list_node<std::coroutine_handle<>>>::iterator storage_location, kphp::coro::async_stack_root& async_stack_root,
             void* return_address) noexcept {
    m_await_broker = await_broker;
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

    auto& instance_state{kphp::coro::instance_state::get()};
    instance_state.note_chain_started();
    instance_state.current_chain_stats = std::addressof(m_chain_stats);

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
  promise_type& m_promise;

  struct await_set_task_promise_common : public await_set_task_promise_base<return_type, promise_type> {
    vk::intrusive::list_node<std::coroutine_handle<>> m_coroutine_node;

    auto get_return_object() noexcept {
      auto& self{static_cast<promise_type&>(*this)};
      m_coroutine_node.value() = std::coroutine_handle<promise_type>::from_promise(self);
      return await_set_task{self};
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
      kphp::log::assertion(m_result.has_value());
      return std::move(*m_result);
    }

    void return_void() noexcept {
      kphp::log::assertion(false);
    }
  };

  struct await_set_task_promise_void : public await_set_task_promise_common {
    constexpr kphp::coro::void_value result() const noexcept {
      return {};
    }

    constexpr void return_void() const noexcept {}
  };

public:
  template<typename T>
  friend class detail::await_set::await_broker;

  explicit await_set_task(promise_type& promise) noexcept
      : m_promise(promise) {}

  await_set_task(const await_set_task&) = delete;
  await_set_task(await_set_task&& other) noexcept = delete;

  await_set_task& operator=(const await_set_task&) = delete;
  await_set_task& operator=(await_set_task&&) = delete;

  ~await_set_task() = default;
};

template<typename return_type>
class await_set_awaitable {
public:
  using result_type = std::conditional_t<std::is_void_v<return_type>, kphp::coro::void_value, std::remove_cvref_t<return_type>>;

private:
  await_broker<return_type>& m_await_broker;

  class awaiter {
    vk::intrusive::list_node<std::coroutine_handle<>> m_awaiting_coroutine_node;
    await_broker<return_type>& m_await_broker;
    kphp::coro::async_stack_frame* caller_frame{};
    kphp::coro::chain_stats* m_chain_stats;

  public:
    explicit awaiter(await_broker<return_type>& await_broker) noexcept
        : m_await_broker(await_broker),
          m_chain_stats(kphp::coro::instance_state::get().current_chain_stats) {}

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
      m_awaiting_coroutine_node.value() = awaiting_coroutine;

      return m_await_broker.suspend_awaiter(m_awaiting_coroutine_node);
    }

    std::optional<result_type> await_resume() noexcept {
      // restore caller async stack frame
      kphp::log::assertion(caller_frame != nullptr);
      auto* async_stack_root{caller_frame->async_stack_root};
      kphp::log::assertion(async_stack_root != nullptr);
      async_stack_root->top_async_stack_frame = caller_frame;
      kphp::coro::instance_state::get().current_chain_stats = m_chain_stats;

      return m_await_broker.try_get_result();
    }

    ~awaiter() = default;
  };

public:
  explicit await_set_awaitable(await_broker<return_type>& await_broker) noexcept
      : m_await_broker(await_broker) {}

  await_set_awaitable(const await_set_awaitable&) = delete;
  await_set_awaitable(await_set_awaitable&& other) noexcept
      : m_await_broker(other.m_await_broker) {}

  await_set_awaitable& operator=(const await_set_awaitable&) = delete;
  await_set_awaitable& operator=(await_set_awaitable&& other) = delete;

  auto operator co_await() noexcept {
    return awaiter{m_await_broker};
  }

  ~await_set_awaitable() = default;
};

template<kphp::coro::concepts::awaitable awaitable_type>
auto make_await_set_task(awaitable_type coroutine) noexcept -> await_set_task<typename kphp::coro::awaitable_traits<awaitable_type>::awaiter_return_type> {
  if constexpr (std::is_void_v<typename kphp::coro::awaitable_traits<awaitable_type>::awaiter_return_type>) {
    co_await std::move(coroutine);
  } else {
    co_yield co_await std::move(coroutine);
  }
}

} // namespace kphp::coro::detail::await_set
