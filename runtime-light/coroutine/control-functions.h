// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <coroutine>
#include <utility>

#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/coroutine/detail/allocator/task-allocator.h"
#include "runtime-light/coroutine/task-allocator-guard.h"

namespace kphp::coro {

/**
 * The `resume` function is responsible for storing the current synchronous stack frame
 * in async_stack_root::stop_sync_frame before resuming the coroutine. This allows
 * capturing one of the stack frames in the synchronous stack trace.
 * All calls to resume() method of handle must be made with this function.
 */
inline void resume(std::coroutine_handle<> handle, async_stack_root& stack_root, kphp::coro::detail::memory::task_allocator& task_allocator) noexcept {
  kphp::coro::task_allocator_guard guard{task_allocator};
  auto* previous_stack_frame{std::exchange(stack_root.stop_sync_stack_frame, reinterpret_cast<stack_frame*>(STACK_FRAME_ADDRESS))};
  handle.resume();
  stack_root.stop_sync_stack_frame = previous_stack_frame;
}

/*
 * All calls to resume() method of handle must be made with this function.
 */
inline void resume(std::coroutine_handle<> handle, kphp::coro::detail::memory::task_allocator& task_allocator) noexcept {
  kphp::coro::task_allocator_guard guard{task_allocator};
  handle.resume();
}

/*
 * All calls to destroy() method of handle must be made with this function (except for task<T> destructor).
 */
inline void destroy(std::coroutine_handle<> handle, kphp::coro::detail::memory::task_allocator& task_allocator) noexcept {
  kphp::coro::task_allocator_guard guard{task_allocator};
  handle.destroy();
}

} // namespace kphp::coro
