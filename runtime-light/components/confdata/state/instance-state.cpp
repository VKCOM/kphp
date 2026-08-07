// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/components/confdata/state/instance-state.h"

#include <cstddef>
#include <memory>
#include <span>
#include <utility>

#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/streams/stream.h"

auto InstanceState::init() noexcept -> void {
  auto main_task{run()};
  // initialize async stack
  auto& main_task_async_stack_frame{main_task.get_handle().promise().get_async_stack_frame()};
  main_task_async_stack_frame.async_stack_root = std::addressof(coroutine_instance_state.coroutine_stack_root);
  coroutine_instance_state.coroutine_stack_root.top_async_stack_frame = std::addressof(main_task_async_stack_frame);
  // spawn main task onto the scheduler
  kphp::log::assertion(io_scheduler.spawn(std::move(main_task)));
}

auto InstanceState::run() noexcept -> kphp::coro::task<> {
  auto opt_stream{co_await kphp::component::stream::accept()};
  if (!opt_stream.has_value()) [[unlikely]] {
    kphp::log::warning("failed to accept a stream");
    co_return;
  }
  auto request_stream{std::move(*opt_stream)};
  kphp::log::info("accepted a stream: descriptor -> {}", request_stream.descriptor());

  // dummy implementation: drain the request and close
  if (auto expected{co_await request_stream.read_all([](std::span<const std::byte>) noexcept {})}; !expected) [[unlikely]] {
    kphp::log::warning("failed to read a request: error -> {}", expected.error());
  }
}
