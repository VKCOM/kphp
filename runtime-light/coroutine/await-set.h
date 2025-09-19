// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-light/coroutine/concepts.h"
#include "runtime-light/coroutine/detail/await-set.h"
#include "runtime-light/coroutine/type-traits.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::coro {

template<typename return_type>
class await_set {
public:
  template<typename T>
  friend class kphp::coro::detail::await_set::await_set_task;

  template<typename T>
  friend class kphp::coro::detail::await_set::await_set_awaitable;

  //todo Is it possible to do without this?
  template<typename T, typename U>
  friend class kphp::coro::detail::await_set::await_set_task_promise_base;

private:
  class await_broker {
    kphp::coro::await_set<return_type>& m_await_set;

    //todo make these lists intrusive
    kphp::stl::list<detail::await_set::await_set_awaitable<return_type>*, kphp::memory::script_allocator> m_waiters{};
    kphp::stl::list<detail::await_set::await_set_task<return_type>*, kphp::memory::script_allocator> m_ready_tasks{};

  public:
    explicit await_broker(kphp::coro::await_set<return_type>& await_set)
        : m_await_set(await_set) {}

    void publish(kphp::coro::detail::await_set::await_set_task<return_type>& task) {
      m_ready_tasks.push_back(std::addressof(task));
      if (!m_waiters.empty()) {
        auto* waiter{m_waiters.front()};
        m_waiters.pop_front();
        waiter->notify_awaitable_completed();
      }
    }

    bool try_wait(kphp::coro::detail::await_set::await_set_awaitable<return_type>& waiter) {
      if (!m_ready_tasks.empty() || m_await_set.empty()) {
        return false;
      }
      m_waiters.push_front(std::addressof(waiter));
      return true;
    }

    std::optional<typename kphp::coro::detail::await_set::await_set_awaitable<return_type>::result_type> try_get_result() {
      if (m_ready_tasks.empty()) {
        return std::nullopt;
      }

      auto* ready_task{m_ready_tasks.front()};
      m_ready_tasks.pop_front();
      auto result{std::move(ready_task->result())};

      auto it = std::find(m_await_set.m_tasks_storage.begin(), m_await_set.m_tasks_storage.end(), *ready_task);
      kphp::log::assertion(it != m_await_set.m_tasks_storage.end());
      m_await_set.m_tasks_storage.erase(it);
      return result;
    }

    void detach_all_waiters() {
      while (!m_waiters.empty()) {
        auto* waiter{m_waiters.front()};
        m_waiters.pop_front();
        waiter->notify_awaitable_completed(); // send handle in scheduler?
      }
    }

    ~await_broker() {
      detach_all_waiters();
    }
  };

  //todo make it as set?
  kphp::stl::list<detail::await_set::await_set_task<return_type>, kphp::memory::script_allocator> m_tasks_storage{};
  await_broker m_await_broker;

public:
  await_set()
      : m_await_broker(*this) {}

  template<typename coroutine_type>
  requires kphp::coro::concepts::coroutine<coroutine_type> && std::is_same_v<typename coroutine_traits<coroutine_type>::return_type, return_type>
  void spawn(coroutine_type coroutine) noexcept {
    m_tasks_storage.push_front(detail::await_set::make_await_set_task(std::move(coroutine)));
    m_tasks_storage.begin()->start(m_await_broker);
  }

  auto next() {
    return detail::await_set::await_set_awaitable<return_type>{m_await_broker};
  }

  void shutdown() {
    m_await_broker.detach_all_waiters();
    m_tasks_storage.clear();
  }

  bool empty() {
    return m_tasks_storage.empty();
  }
};

} // namespace kphp::coro
