// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <coroutine>
#include <utility>

#include "runtime-common/core/memory-resource/segmented-stack-resource.h"
#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/coroutine/detail/allocator/task-allocator.h"

namespace kphp::coro {

/**
 * The `resume` function is responsible for storing the current synchronous stack frame
 * in async_stack_root::stop_sync_frame before resuming the coroutine. This allows
 * capturing one of the stack frames in the synchronous stack trace.
 * Also this function is responsible for switching of the current task allocator.
 */
inline void resume(std::coroutine_handle<> handle, async_stack_root& stack_root,
                   memory_resource::segmented_stack_resource<kphp::coro::detail::memory::task_allocator::shared_chunk_pool>& task_memory_resource) noexcept {
  auto* previous_stack_frame{std::exchange(stack_root.stop_sync_stack_frame, reinterpret_cast<stack_frame*>(STACK_FRAME_ADDRESS))};
  auto* previous_task_memory_resource{kphp::coro::detail::memory::task_allocator::get().exchange(std::addressof(task_memory_resource))};
  handle.resume();
  kphp::coro::detail::memory::task_allocator::get().set(previous_task_memory_resource);
  stack_root.stop_sync_stack_frame = previous_stack_frame;
}

/**
 * The `resume` function is responsible for switching of the current task allocator.
 */
inline void resume(std::coroutine_handle<> handle,
                   memory_resource::segmented_stack_resource<kphp::coro::detail::memory::task_allocator::shared_chunk_pool>& task_memory_resource) noexcept {
  auto* previous_task_memory_resource{kphp::coro::detail::memory::task_allocator::get().exchange(std::addressof(task_memory_resource))};
  handle.resume();
  kphp::coro::detail::memory::task_allocator::get().set(previous_task_memory_resource);
}

/**
 * The `resume` function is responsible for storing the current synchronous stack frame
 * in async_stack_root::stop_sync_frame before resuming the coroutine. This allows
 * capturing one of the stack frames in the synchronous stack trace.
 */
inline void resume(std::coroutine_handle<> handle, async_stack_root& stack_root) noexcept {
  auto* previous_stack_frame{std::exchange(stack_root.stop_sync_stack_frame, reinterpret_cast<stack_frame*>(STACK_FRAME_ADDRESS))};
  handle.resume();
  stack_root.stop_sync_stack_frame = previous_stack_frame;
}

/**
 * The `destroy` function is responsible for switching of the current task allocator.
 */
inline void destroy(std::coroutine_handle<> handle,
                    memory_resource::segmented_stack_resource<kphp::coro::detail::memory::task_allocator::shared_chunk_pool>& task_memory_resource) noexcept {
  auto* previous_task_memory_resource{kphp::coro::detail::memory::task_allocator::get().exchange(std::addressof(task_memory_resource))};
  handle.destroy();
  kphp::coro::detail::memory::task_allocator::get().set(previous_task_memory_resource);
}

} // namespace kphp::coro
