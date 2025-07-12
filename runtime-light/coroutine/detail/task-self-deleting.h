// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <coroutine>
#include <cstddef>

#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/utils/logs.h"

namespace kphp::coro::detail {

class task_self_deleting;

namespace task_self_deleting_impl {

struct promise_self_deleting : kphp::coro::async_stack_element { // TODO async stack support
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
  auto initial_suspend() const noexcept -> std::suspend_always;
  auto final_suspend() const noexcept -> std::suspend_never;
  auto return_void() const noexcept -> void;
  auto unhandled_exception() const noexcept -> void;
};

}; // namespace task_self_deleting_impl

class task_self_deleting {
  task_self_deleting_impl::promise_self_deleting& m_promise;

public:
  using promise_type = task_self_deleting_impl::promise_self_deleting;

  explicit task_self_deleting(task_self_deleting_impl::promise_self_deleting& promise) noexcept
      : m_promise(promise) {}
  ~task_self_deleting() noexcept = default;

  task_self_deleting(const task_self_deleting&) = delete;
  task_self_deleting(task_self_deleting&&) noexcept = default;

  task_self_deleting& operator=(const task_self_deleting&) = delete;
  task_self_deleting& operator=(task_self_deleting&&) = delete;

  auto get_handle() noexcept -> std::coroutine_handle<task_self_deleting_impl::promise_self_deleting>;
};

inline auto task_self_deleting_impl::promise_self_deleting::get_return_object() noexcept -> task_self_deleting {
  return task_self_deleting{*this};
}

inline auto task_self_deleting_impl::promise_self_deleting::initial_suspend() const noexcept -> std::suspend_always {
  return {};
}

inline auto task_self_deleting_impl::promise_self_deleting::final_suspend() const noexcept -> std::suspend_never {
  return {};
}

inline auto task_self_deleting_impl::promise_self_deleting::return_void() const noexcept -> void {}

inline auto task_self_deleting_impl::promise_self_deleting::unhandled_exception() const noexcept -> void {
  kphp::log::error("internal unhandled exception");
}

inline auto task_self_deleting::get_handle() noexcept -> std::coroutine_handle<task_self_deleting_impl::promise_self_deleting> {
  return std::coroutine_handle<task_self_deleting_impl::promise_self_deleting>::from_promise(m_promise);
}

// ================================================================================================

inline auto make_task_self_deleting(kphp::coro::task<> task) noexcept -> task_self_deleting {
  co_await task;
  co_return;
}

} // namespace kphp::coro::detail
