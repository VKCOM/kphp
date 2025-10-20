// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <memory>
#include <type_traits>

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
  std::unique_ptr<detail::await_set::await_broker<return_type>> m_await_broker;
  kphp::coro::async_stack_root& m_coroutine_stack_root;

  template<typename T>
  friend class kphp::coro::detail::await_set::await_broker;

  void clear() noexcept {
    m_await_broker.release();
    m_tasks_storage.clear();
  }

public:
  explicit await_set() noexcept
      : m_await_broker(std::make_unique<detail::await_set::await_broker<return_type>>(*this)),
        m_coroutine_stack_root(CoroutineInstanceState::get().coroutine_stack_root) {}

  await_set(await_set&& other) noexcept
      : m_tasks_storage(std::move(other.m_tasks_storage)),
        m_await_broker(std::move(other.m_await_broker)),
        m_coroutine_stack_root(other.m_coroutine_stack_root) {}

  await_set& operator=(await_set&& other) noexcept {
    if (this != std::addressof(other)) {
      clear();
      m_tasks_storage = std::move(other.m_tasks_storage);
      m_await_broker = std::move(other.m_await_broker);
      m_coroutine_stack_root = other.m_coroutine_stack_root;
    }
    return *this;
  }

  await_set(const await_set&) = delete;
  await_set& operator=(const await_set&) = delete;

  template<typename awaitable_type>
  requires kphp::coro::concepts::awaitable<awaitable_type> && std::is_same_v<typename awaitable_traits<awaitable_type>::awaiter_return_type, return_type>
  void push(awaitable_type awaitable) noexcept {
    auto task_iterator{m_tasks_storage.insert(m_tasks_storage.begin(), detail::await_set::make_await_set_task(std::move(awaitable)))};
    task_iterator->start(*m_await_broker.get(), task_iterator, m_coroutine_stack_root, STACK_RETURN_ADDRESS);
  }

  auto next() noexcept {
    return detail::await_set::await_set_awaitable<return_type>{*m_await_broker.get()};
  }

  bool empty() const noexcept {
    return len() == 0;
  }

  size_t len() const noexcept {
    return m_tasks_storage.size();
  }

  ~await_set() {
    clear();
  }
};

} // namespace kphp::coro
