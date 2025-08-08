// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <coroutine>
#include <cstddef>
#include <memory>

#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/coroutine/concepts.h"
#include "runtime-light/coroutine/coroutine-state.h"
#include "runtime-light/stdlib/diagnostics/diagnostics.h"

namespace kphp::coro::detail {

namespace task_self_deleting {

class task_self_deleting;

struct promise_self_deleting : kphp::coro::async_stack_element {
  promise_self_deleting() noexcept = default;
  ~promise_self_deleting() = default;

  promise_self_deleting(const promise_self_deleting&) = delete;
  promise_self_deleting(promise_self_deleting&&) noexcept = default;

  promise_self_deleting& operator=(const promise_self_deleting&) = delete;
  promise_self_deleting& operator=(promise_self_deleting&&) = delete;

  template<typename... Args>
  auto operator new(size_t n, [[maybe_unused]] Args&&... args) noexcept -> void* {
    return kphp::memory::script::alloc(n);
  }

  auto operator delete(void* ptr, [[maybe_unused]] size_t n) noexcept -> void {
    kphp::memory::script::free(ptr);
  }

  auto get_return_object() noexcept -> task_self_deleting;

  static auto get_return_object_on_allocation_failure() noexcept -> task_self_deleting;

  auto initial_suspend() const noexcept -> std::suspend_always {
    return {};
  }

  auto final_suspend() const noexcept -> std::suspend_never {
    return {};
  }

  auto return_void() const noexcept -> void {}

  auto unhandled_exception() const noexcept -> void {
    kphp::log::error("internal unhandled exception");
  }
};

class task_self_deleting {
  promise_self_deleting& m_promise;

public:
  using promise_type = promise_self_deleting;

  explicit task_self_deleting(promise_self_deleting& promise) noexcept
      : m_promise(promise) {
    // initialize task_self_deleting's frame as top async stack frame
    auto& async_stack_frame{m_promise.get_async_stack_frame()};
    async_stack_frame.return_address = STACK_RETURN_ADDRESS;
    async_stack_frame.async_stack_root = std::addressof(CoroutineInstanceState::get().coroutine_stack_root);
  }
  ~task_self_deleting() noexcept = default;

  task_self_deleting(const task_self_deleting&) = delete;
  task_self_deleting(task_self_deleting&&) noexcept = default;

  task_self_deleting& operator=(const task_self_deleting&) = delete;
  task_self_deleting& operator=(task_self_deleting&&) = delete;

  auto get_handle() noexcept -> std::coroutine_handle<promise_self_deleting> {
    return std::coroutine_handle<promise_type>::from_promise(m_promise);
  }
};

inline auto promise_self_deleting::get_return_object() noexcept -> task_self_deleting {
  return task_self_deleting{*this};
}

inline auto promise_self_deleting::get_return_object_on_allocation_failure() noexcept -> task_self_deleting {
  kphp::log::error("cannot allocate memory for task_self_deleting");
}

} // namespace task_self_deleting

template<kphp::coro::concepts::awaitable awaitable_type>
auto make_task_self_deleting(awaitable_type awaitable) noexcept -> task_self_deleting::task_self_deleting {
  co_await std::move(awaitable);
  co_return;
}

} // namespace kphp::coro::detail
