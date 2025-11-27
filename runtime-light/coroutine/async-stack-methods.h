// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-light/coroutine/async-stack.h"

#include <coroutine>
#include <utility>

#include "runtime-light/coroutine/coroutine-state.h"

namespace kphp::coro {

/**
 * The `resume_with_new_root` function is responsible for storing the current synchronous stack frame
 * in async_stack_root::stop_sync_frame before resuming the coroutine. This allows
 * capturing one of the stack frames in the synchronous stack trace and jump between parts of the async_stack.
 */
[[clang::noinline]] inline void resume_with_new_root(std::coroutine_handle<> handle, async_stack_root* new_async_stack_root) noexcept {
  auto& coroutine_st{CoroutineInstanceState::get()};
  auto* previous_stack_root = coroutine_st.current_async_stack_root;
  new_async_stack_root->next_async_stack_root = coroutine_st.current_async_stack_root;

  new_async_stack_root->stop_sync_stack_frame = reinterpret_cast<stack_frame*>(STACK_FRAME_ADDRESS);
  coroutine_st.current_async_stack_root = new_async_stack_root;
  handle.resume();
  coroutine_st.current_async_stack_root = previous_stack_root;
}
} // namespace kphp::coro
