//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2025 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>

#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/coroutine/coroutine-state.h"
#include "runtime-light/stdlib/diagnostics/stacktrace.h"

#include "runtime-light/k2-platform/k2-api.h"

namespace {

std::ptrdiff_t get_code_segment_offset() {
  uint64_t offset{};
  auto res = k2::code_segment_offset(&offset);
  return res == 0 ? offset : 0;
}

std::size_t normal_frames(std::span<void*> addresses, kphp::coro::stack_frame* start_frame, kphp::coro::stack_frame* stop_frame) {
  php_assert(start_frame != nullptr && stop_frame != nullptr);

  auto address{addresses.begin()};
  const auto code_segment_offset(get_code_segment_offset());
  for (auto* current_stack_frame{start_frame}; current_stack_frame != stop_frame && address != addresses.end();
       current_stack_frame = current_stack_frame->caller_frame) {
    *address = (reinterpret_cast<std::byte*>(current_stack_frame->return_address) - code_segment_offset);
    address = std::next(address);
  }
  return std::distance(addresses.begin(), address);
}

std::size_t coroutine_frames(std::span<void*> addresses, kphp::coro::async_stack_frame* stack_frame) {
  php_assert(stack_frame != nullptr);

  auto address{addresses.begin()};
  const auto code_segment_offset(get_code_segment_offset());
  for (auto* current_async_frame{stack_frame}; current_async_frame != nullptr && address != addresses.end();
       current_async_frame = current_async_frame->caller_async_frame) {
    *address = (reinterpret_cast<std::byte*>(current_async_frame->return_address) - code_segment_offset);
    address = std::next(address);
  }
  return std::distance(addresses.begin(), address);
}

} // namespace

namespace kphp::diagnostic {

std::size_t get_async_stacktrace(std::span<void*> addresses) {
  auto& async_stack_root{CoroutineInstanceState::get().get_coroutine_stack_root()};

  auto* const normal_stack_frame{reinterpret_cast<kphp::coro::stack_frame*>(__builtin_frame_address(0))};
  auto* const normal_stack_frame_stop{async_stack_root.stack_frame};

  const std::size_t num_frames{normal_frames(addresses, normal_stack_frame, normal_stack_frame_stop)};
  const std::size_t num_async_frames{coroutine_frames(addresses.subspan(num_frames), async_stack_root.top_frame)};
  return num_frames + num_async_frames;
}

} // namespace kphp::diagnostic
