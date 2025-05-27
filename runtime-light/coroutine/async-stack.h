// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <coroutine>
#include <utility>

/**
 * This header defines the data structures used to represent a coroutine asynchronous stack.
 *
 * Overview:
 * The asynchronous stack is used to manage the execution state of coroutines, allowing for
 * efficient context switching and stack management.
 *
 * Diagram: Normal and Asynchronous Stacks
 *
 *  Base Pointer (%rbp)              async_stack_root
 *         |                                 |
 *         V                                 V
 *    stack_frame                    async_stack_frame
 *         |                                 |
 *         V                                 V
 *    stack_frame                    async_stack_frame
 *        ...                               ...
 *         |                                 |
 *         V                                 V
 *    stack_frame                    async_stack_frame
 *         |                                 |
 *         V                                 V
 *
 * In the diagram above, the left side represents a typical call stack with stack frames linked by
 * the base pointer (%rbp). The right side illustrates an asynchronous stack where `async_stack_frame`
 * structures are linked by `async_stack_root`.
 *
 * Diagram: Backtrace Mechanism
 *
 *  Base Pointer (%rbp)
 *         |
 *         V
 *    stack_frame
 *         |
 *         V
 *    stack_frame (stop_sync_frame)  <- async_stack_root
 *                                             |
 *                                             V
 *                                     async_stack_frame (top_async_frame)
 *                                             |
 *                                             V
 *                                     async_stack_frame
 *                                            ...
 *                                             |
 *                                             V
 *                                     async_stack_frame
 *
 * The backtrace mechanism involves traversing the stack frames to capture the call stack.
 * The `stop_sync_frame` serves as a marker where the transition to the asynchronous stack occurs,
 * allowing the backtrace to continue through the `async_stack_frame` structures.
 */

#define STACK_RETURN_ADDRESS __builtin_return_address(0)

#define STACK_FRAME_ADDRESS __builtin_frame_address(0)

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
  async_stack_frame* top_async_frame{};
  stack_frame* stop_sync_frame{};
};

/**
 * The `resume` function is responsible for storing the current synchronous stack frame
 * in async_stack_root::stop_sync_frame before resuming the coroutine. This allows
 * capturing one of the stack frames in the synchronous stack trace.
 */
inline void resume(std::coroutine_handle<> handle, async_stack_root& stack_root) noexcept {
  auto* previous_stack_frame{std::exchange(stack_root.stop_sync_frame, reinterpret_cast<stack_frame*>(STACK_FRAME_ADDRESS))};
  handle.resume();
  stack_root.stop_sync_frame = previous_stack_frame;
}

/**
 * The async_stack_element class facilitates working with asynchronous traces in templated code.
 * This allows for uniform handling of any coroutines in places where async frames are pushed or popped.
 */
struct async_stack_element {
  async_stack_frame& get_async_stack_frame() noexcept {
    return async_stack_frame_;
  }

private:
  async_stack_frame async_stack_frame_;
};

} // namespace kphp::coro
