//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2025 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstddef>
#include <iterator>

#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/coroutine/async-frame.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/stdlib/diagnostics/stacktrace.h"

namespace {

std::size_t get_normal_stack_part(void** addresses, std::size_t max_addresses, kphp::coro::stack_frame* start_frame, kphp::coro::stack_frame* stop_frame) {
  php_assert(stop_frame != nullptr);
  auto* current_stack_frame{start_frame};
  std::size_t number{};
  while (current_stack_frame != stop_frame && number < max_addresses) {
    addresses[number] = reinterpret_cast<void*>(current_stack_frame->return_address);
    current_stack_frame = current_stack_frame->caller_frame;
    number++;
  }

  return number;
}

std::size_t get_async_stack_part(void** addresses, size_t max_addresses, kphp::coro::async_stack_frame* stack_frame) {
  auto* current_async_frame{stack_frame};
  std::size_t number{};
  while (current_async_frame != nullptr && number < max_addresses) {
    addresses[number] = reinterpret_cast<void*>(current_async_frame->return_address);
    current_async_frame = current_async_frame->caller_async_frame;
    number++;
  }
  return number;
}

} // namespace

namespace kphp::diagnostic {

std::size_t get_async_stacktrace(void** data, std::size_t max_addresses) {

  auto async_stack_root{InstanceState::get().coroutine_stack_root};

  auto* normal_stack_frame{reinterpret_cast<kphp::coro::stack_frame*>(__builtin_frame_address(0))};
  auto* normal_stack_frame_stop{async_stack_root.stack_frame};

  std::size_t num_frames = get_normal_stack_part(data, max_addresses, normal_stack_frame, normal_stack_frame_stop);
  std::advance(data, num_frames);
  std::size_t num_async_frames = get_async_stack_part(data, max_addresses - num_frames, async_stack_root.top_frame);

  return num_frames + num_async_frames;
}

} // namespace kphp::diagnostic
