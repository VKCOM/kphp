// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-light/coroutine/async-stack.h"

#include <coroutine>
#include <utility>

#include "runtime-light/coroutine/coroutine-state.h"

namespace kphp::coro {

using ::CoroutineInstanceState;

/**
 * The `resume` function is responsible for storing the current synchronous stack frame
 * in async_stack_root::stop_sync_frame before resuming the coroutine and also creates a new async_stack_root. This allows
 * capturing one of the stack frames in the synchronous stack trace and jump between parts of the async_stack.
 */
[[clang::noinline]] inline void resume(std::coroutine_handle<> handle, async_stack_root* stack_root = nullptr) noexcept {
  if (stack_root == nullptr) {
    stack_root = CoroutineInstanceState::get_next_root();
  }
  async_stack_root root{};
  root.next_async_stack_root = stack_root;
  auto* previous_stack_frame{std::exchange(stack_root->stop_sync_stack_frame, reinterpret_cast<stack_frame*>(STACK_FRAME_ADDRESS))};

  CoroutineInstanceState::set_next_root(std::addressof(root));
  handle.resume();
  stack_root->stop_sync_stack_frame = previous_stack_frame;
  CoroutineInstanceState::set_next_root(stack_root);
}
} // namespace kphp::coro
