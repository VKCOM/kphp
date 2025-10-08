// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/coroutine/concepts.h"
#include "runtime-light/coroutine/coroutine-state.h"
#include "runtime-light/coroutine/detail/await-set.h"
#include "runtime-light/coroutine/type-traits.h"

namespace kphp::coro {

template<typename return_type>
class await_set {
  kphp::stl::list<detail::await_set::await_set_task<return_type>, kphp::memory::script_allocator> m_tasks_storage{};
  detail::await_set::await_broker<return_type> m_await_broker;

public:
  template<typename T>
  friend class kphp::coro::detail::await_set::await_broker;

  explicit await_set(await_set_waiting_policy waiting_policy = await_set_waiting_policy::suspend_on_empty) noexcept
      : m_await_broker(*this, waiting_policy) {}

  await_set(const await_set&) = delete;
  await_set(await_set&&) = delete;

  await_set& operator=(const await_set&) = delete;
  await_set& operator=(await_set&& other) = delete;

  template<typename awaitable_type>
  requires kphp::coro::concepts::awaitable<awaitable_type> && std::is_same_v<typename awaitable_traits<awaitable_type>::awaiter_return_type, return_type>
  void push(awaitable_type awaitable) noexcept {
    m_tasks_storage.push_front(detail::await_set::make_await_set_task(std::move(awaitable)));
    auto task_iterator{m_tasks_storage.begin()};
    auto& coroutine_stack_root{CoroutineInstanceState::get().coroutine_stack_root};
    task_iterator->start(m_await_broker, task_iterator, coroutine_stack_root, STACK_RETURN_ADDRESS);
  }

  auto next() noexcept {
    return detail::await_set::await_set_awaitable<return_type>{m_await_broker};
  }

  void abort_all() noexcept {
    m_await_broker.detach_awaiters();
    m_tasks_storage.clear();
  }

  bool empty() const noexcept {
    return len() == 0;
  }

  size_t len() const noexcept {
    return m_tasks_storage.size();
  }
};

} // namespace kphp::coro
