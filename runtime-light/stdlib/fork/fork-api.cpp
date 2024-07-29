// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/fork/fork-api.h"

#include <chrono>
#include <cstdint>

#include "runtime-core/utils/kphp-assert-core.h"
#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/coroutine/task.h"

task_t<void> f$sched_yield() noexcept {
  co_await wait_for_reschedule_t{};
}

task_t<void> f$sched_yield_sleep(int64_t duration_ns) noexcept {
  if (duration_ns < 0) {
    php_warning("can't sleep for negative duration %" PRId64, duration_ns);
    co_return;
  }
  co_await wait_for_timer_t{std::chrono::nanoseconds{static_cast<uint64_t>(duration_ns)}};
}
