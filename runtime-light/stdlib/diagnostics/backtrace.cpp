//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2025 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstddef>
#include <iterator>
#include <span>

#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/coroutine/coroutine-state.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/stdlib/diagnostics/backtrace.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace {

size_t sync_frames(std::span<void*> addresses, const kphp::coro::stack_frame* start_frame, const kphp::coro::stack_frame* stop_frame) noexcept {
  kphp::log::assertion(start_frame != nullptr && stop_frame != nullptr);

  auto address{addresses.begin()};
  for (const auto* current_stack_frame{start_frame}; current_stack_frame != stop_frame && address != addresses.end();
       current_stack_frame = current_stack_frame->caller_stack_frame) {
    *address = current_stack_frame->return_address;
    address = std::next(address);
  }
  return std::distance(addresses.begin(), address);
}

size_t async_frames(std::span<void*> addresses, kphp::coro::async_stack_frame* top_async_frame) noexcept {
  kphp::log::assertion(top_async_frame != nullptr);

  auto address{addresses.begin()};
  for (auto* current_async_frame{top_async_frame}; current_async_frame != nullptr && address != addresses.end();
       current_async_frame = current_async_frame->caller_async_stack_frame) {
    *address = current_async_frame->return_address;
    address = std::next(address);
  }
  return std::distance(addresses.begin(), address);
}

} // namespace

namespace kphp::diagnostic::impl {

size_t async_backtrace_helper(std::span<void*> addresses, const kphp::coro::stack_frame* start_sync_frame,
                              const kphp::coro::async_stack_root* async_stack_root) noexcept {
  if (async_stack_root == nullptr) {
    return 0;
  }
  const auto* const stop_sync_frame{async_stack_root->stop_sync_stack_frame};

  const size_t num_sync_frames{sync_frames(addresses, start_sync_frame, stop_sync_frame)};
  const size_t num_async_frames{async_frames(addresses.subspan(num_sync_frames), async_stack_root->top_async_stack_frame)};

  const size_t result{num_sync_frames + num_async_frames};
  if (addresses.size() < result)
    return result;
  return result + async_backtrace_helper(addresses.subspan(result), stop_sync_frame, async_stack_root->next_async_stack_root);
}

size_t async_backtrace(std::span<void*> addresses) noexcept {
  if (const auto* instance_state{k2::instance_state()}; instance_state != nullptr) [[likely]] {
    return async_backtrace_helper(addresses, reinterpret_cast<kphp::coro::stack_frame*>(STACK_FRAME_ADDRESS),
                                  instance_state->coroutine_instance_state.next_root->next_async_stack_root);
  }
  return 0;
}
} // namespace kphp::diagnostic::impl
