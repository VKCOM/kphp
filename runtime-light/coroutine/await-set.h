// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-light/coroutine/concepts.h"
#include "runtime-light/coroutine/detail/await-set.h"
#include "runtime-light/coroutine/type-traits.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/coroutine/coroutine-state.h"
#include "runtime-light/coroutine/async-stack.h"

namespace kphp::coro {

template<typename return_type>
class await_set {
  kphp::stl::list<detail::await_set::await_set_task<return_type>, kphp::memory::script_allocator> m_tasks_storage{};
  detail::await_set::await_broker<return_type> m_await_broker;

public:
  template<typename T>
  friend class kphp::coro::detail::await_set::await_broker;

  await_set()
      : m_await_broker(*this) {}

  template<typename awaitable_type>
  requires kphp::coro::concepts::awaitable<awaitable_type> && std::is_same_v<typename awaitable_traits<awaitable_type>::awaiter_return_type, return_type>
  void push(awaitable_type awaitable) noexcept {
    m_tasks_storage.push_front(detail::await_set::make_await_set_task(std::move(awaitable)));
    auto it{m_tasks_storage.begin()};
    auto &coroutine_stack_root{CoroutineInstanceState::get().coroutine_stack_root};
    it->start(m_await_broker, it, coroutine_stack_root, STACK_RETURN_ADDRESS);
  }

  auto next() {
    return detail::await_set::await_set_awaitable<return_type>{m_await_broker};
  }

  void abort_all() {
    m_await_broker.detach_awaiters();
    m_tasks_storage.clear();
  }

  bool empty() {
    return m_tasks_storage.empty();
  }

  size_t len() {
    return m_tasks_storage.size();
  }
};

} // namespace kphp::coro
