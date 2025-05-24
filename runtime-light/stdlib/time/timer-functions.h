// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <concepts>
#include <cstdint>
#include <functional>
#include <utility>

#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/utils/logs.h"

template<std::invocable T>
kphp::coro::task<> f$set_timer(int64_t timeout_ms, T on_timer_callback) noexcept {
  if (timeout_ms < 0) [[unlikely]] {
    kphp::log::warning("can't set timer for negative duration {} ms", timeout_ms);
    co_return;
  }
  const auto timer_f{[](std::chrono::nanoseconds duration, T on_timer_callback) noexcept -> kphp::coro::task<> {
    co_await wait_for_timer_t{duration};
    std::invoke(std::move(on_timer_callback));
  }}; // TODO: someone should pop that fork from ForkComponentContext since it will stay there unless we perform f$wait on fork
  const auto duration_ms{std::chrono::milliseconds{static_cast<uint64_t>(timeout_ms)}};
  co_await start_fork_t{std::invoke(timer_f, std::chrono::duration_cast<std::chrono::nanoseconds>(duration_ms), std::move(on_timer_callback))};
}
