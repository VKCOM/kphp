// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/coroutine/await-set-enums.h"
#include "runtime-light/coroutine/concepts.h"
#include "runtime-light/coroutine/coroutine-state.h"
#include "runtime-light/coroutine/detail/await-set.h"
#include "runtime-light/coroutine/type-traits.h"

namespace kphp::coro {

template<typename return_type, await_set_waiting_policy waiting_policy = await_set_waiting_policy::resume_on_empty>
class await_set {
  enum class running_state_t : uint8_t {
    running,
    stopped,
  };

  kphp::stl::list<detail::await_set::await_set_task<return_type, waiting_policy>, kphp::memory::script_allocator> m_tasks_storage{};
  detail::await_set::await_broker<return_type, waiting_policy> m_await_broker;
  kphp::coro::async_stack_root& m_coroutine_stack_root;
  running_state_t m_running_state;

  template<typename T, await_set_waiting_policy>
  friend class kphp::coro::detail::await_set::await_broker;

public:
  explicit await_set() noexcept
      : m_await_broker(*this),
        m_coroutine_stack_root(CoroutineInstanceState::get().coroutine_stack_root),
        m_running_state(running_state_t::running) {}

  await_set(const await_set&) = delete;
  await_set(await_set&&) = delete;

  await_set& operator=(const await_set&) = delete;
  await_set& operator=(await_set&& other) = delete;

  template<typename awaitable_type>
  requires kphp::coro::concepts::awaitable<awaitable_type> && std::is_same_v<typename awaitable_traits<awaitable_type>::awaiter_return_type, return_type>
  await_set_push_result_t push(awaitable_type awaitable) noexcept {
    if (m_running_state == running_state_t::stopped) {
      return await_set_push_result_t::stopped;
    }
    auto task_iterator{m_tasks_storage.insert(m_tasks_storage.begin(), detail::await_set::make_await_set_task<waiting_policy>(std::move(awaitable)))};
    task_iterator->start(m_await_broker, task_iterator, m_coroutine_stack_root, STACK_RETURN_ADDRESS);
    return await_set_push_result_t::ok;
  }

  auto next() noexcept {
    return detail::await_set::await_set_awaitable<return_type, waiting_policy>{m_await_broker};
  }

  void abort_all() noexcept {
    if (m_running_state == running_state_t::stopped) {
      return;
    }
    m_running_state = running_state_t::stopped;
    m_await_broker.abort_all();
    m_tasks_storage.clear();
    // Since the abort_all function has successfully completed,
    // it means that no one is waiting for the await set.
    // We switch the await set to the running state to continue working.
    m_running_state = running_state_t::running;
  }

  bool empty() const noexcept {
    return len() == 0;
  }

  size_t len() const noexcept {
    return m_tasks_storage.size();
  }

  ~await_set() {
    abort_all();
  }
};

} // namespace kphp::coro
