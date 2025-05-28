//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2025 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>

#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/coroutine/coroutine-state.h"
#include "runtime-light/stdlib/diagnostics/backtrace.h"
#include "runtime-light/utils/logs.h"

namespace {

size_t sync_frames(std::span<void*> addresses, kphp::coro::stack_frame* start_frame, kphp::coro::stack_frame* stop_frame) noexcept {
  kphp::log::assertion(start_frame != nullptr && stop_frame != nullptr);

  auto address{addresses.begin()};
  for (auto* current_stack_frame{start_frame}; current_stack_frame != stop_frame && address != addresses.end();
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

namespace kphp::diagnostic {

size_t backtrace(std::span<void*> addresses) noexcept {
  auto& async_stack_root{CoroutineInstanceState::get().coroutine_stack_root};

  auto* const start_sync_frame{reinterpret_cast<kphp::coro::stack_frame*>(STACK_FRAME_ADDRESS)};
  auto* const stop_sync_frame{async_stack_root.stop_sync_stack_frame};

  const size_t num_sync_frames{sync_frames(addresses, start_sync_frame, stop_sync_frame)};
  const size_t num_async_frames{async_frames(addresses.subspan(num_sync_frames), async_stack_root.top_async_stack_frame)};
  return num_sync_frames + num_async_frames;
}

} // namespace kphp::diagnostic
