// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <coroutine>
#include <utility>

/*
 * This header defines data-structures used to represent a coroutine async stack.
 *
 * In general terms, the picture looks as follows:
 *
 * Register Base Pointer
 *         |
 *         V
 *    stack_frame     coroutine_stack_root (CoroutineInstanceState)
 *         |                   |
 *         V                   V
 *    stack_frame   <- async_stack_root -> async_stack_frame -> async_stack_frame -> ...
 *        ...
 *         |
 *         V
 *    stack_frame
 *         |
 *         V
 *
 *
 * Since coroutine_stack_root needs to know the pointer to one of the regular frames in the call stack,
 * The resumption of coroutines occurs through the resume_on_async_stack function,
 * whose frame is stored in async_stack_root.
 *
 *
 * async stack frames are stored in coroutines promise type by inheriting from a class stack_element
 *
 * */

namespace kphp::coro {

struct stack_frame {
  stack_frame* caller_frame{};
  void* return_address{};
};

struct async_stack_root;

struct async_stack_frame {
  async_stack_frame* caller_async_frame{};
  async_stack_root* async_stack_root{};
  void* return_address{};
};

struct async_stack_root {
  async_stack_frame* top_frame{};
  stack_frame* stack_frame{};
};

inline void resume_on_async_stack(std::coroutine_handle<> handle, async_stack_root& stack_root) {
  auto* previous_stack_frame{std::exchange(stack_root.stack_frame, reinterpret_cast<stack_frame*>(__builtin_frame_address(0)))};
  handle.resume();
  stack_root.stack_frame = previous_stack_frame;
}

namespace async_stack_impl {

struct stack_element {
  async_stack_frame& get_async_frame() noexcept {
    return async_frame;
  }

private:
  async_stack_frame async_frame;
};

} // namespace async_stack_impl

} // namespace kphp::coro
