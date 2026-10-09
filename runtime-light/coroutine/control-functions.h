// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <coroutine>
#include <utility>

#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/coroutine/detail/allocator/task-allocator.h"

namespace kphp::coro {

/**
 * The `resume` function is responsible for storing the current synchronous stack frame
 * in async_stack_root::stop_sync_frame before resuming the coroutine. This allows
 * capturing one of the stack frames in the synchronous stack trace.
 * All calls to resume() method of handle must be made with overload of this function.
 */
inline void resume(std::coroutine_handle<> handle, async_stack_root& stack_root, kphp::coro::detail::memory::task_allocator& task_allocator) noexcept {
  auto* prev_stack{task_allocator.current_stack()};
  auto* previous_stack_frame{std::exchange(stack_root.stop_sync_stack_frame, reinterpret_cast<stack_frame*>(STACK_FRAME_ADDRESS))};
  handle.resume();
  stack_root.stop_sync_stack_frame = previous_stack_frame;
  task_allocator.set_stack(prev_stack);
}

/*
 * All calls to resume() method of handle must be made with overload of this function.
 */
inline void resume(std::coroutine_handle<> handle, kphp::coro::detail::memory::task_allocator& task_allocator) noexcept {
  auto* prev_stack{task_allocator.current_stack()};
  handle.resume();
  task_allocator.set_stack(prev_stack);
}

/*
 * All calls to destroy() method of handle must be made with this function (except for detached_task<T> destructor).
 */
inline void destroy(std::coroutine_handle<> handle, kphp::coro::detail::memory::task_allocator& task_allocator) noexcept {
  auto* prev_stack{task_allocator.current_stack()};
  handle.destroy();
  task_allocator.set_stack(prev_stack);
}

} // namespace kphp::coro
