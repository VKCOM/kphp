// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-light/coroutine/async-stack-structs.h"

#include <coroutine>
#include <utility>

#include "runtime-light/coroutine/coroutine-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::coro {

struct async_stack_root_wrapper {
  async_stack_root root;
  async_stack_root_wrapper() = default;
  async_stack_root_wrapper(const async_stack_root_wrapper& other) = default;
  async_stack_root_wrapper& operator=(const async_stack_root_wrapper& other) = default;
  ~async_stack_root_wrapper() {
    CoroutineInstanceState::get().current_async_stack_root = root.next_async_stack_root;
  }
};

inline void preparation_for_resume(kphp::coro::async_stack_root* root, void* stack_frame_addr) {
  static CoroutineInstanceState& coroutine_state = CoroutineInstanceState::get();
  root->stop_sync_stack_frame = reinterpret_cast<stack_frame*>(stack_frame_addr);
  coroutine_state.current_async_stack_root = root;
}

/**
 * The `resume_with_new_root` function is responsible for storing the current synchronous stack frame
 * in async_stack_root::stop_sync_frame before resuming the coroutine. This allows
 * capturing one of the stack frames in the synchronous stack trace and jump between parts of the async_stack.
 */
[[clang::noinline]] inline void resume_with_new_root(std::coroutine_handle<> handle, async_stack_root* new_async_stack_root) noexcept {
  kphp::log::assertion(new_async_stack_root != nullptr);
  auto& coroutine_st{CoroutineInstanceState::get()};

  new_async_stack_root->next_async_stack_root = coroutine_st.current_async_stack_root;
  new_async_stack_root->stop_sync_stack_frame = reinterpret_cast<stack_frame*>(STACK_FRAME_ADDRESS);

  coroutine_st.current_async_stack_root = new_async_stack_root;
  handle.resume();
  new_async_stack_root->stop_sync_stack_frame = nullptr;
}
} // namespace kphp::coro
