// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/fork/fork-api.h"

#include <chrono>
#include <cstdint>

#include "runtime-core/utils/kphp-assert-core.h"
#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/fork/wait-queue-context.h"

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

task_t<Optional<int64_t>> f$wait_queue_next(int64_t queue_id, double timeout) noexcept {
  static_assert(CancellableAwaitable<wait_queue_t::awaiter_t>);
  auto queue = WaitQueueContext::get().get_queue(queue_id);
  if (!queue.has_value()) {
    co_return Optional<int64_t>{};
  }
  const auto timeout_ns{timeout > 0 && timeout <= fork_api_impl_::MAX_TIMEOUT_S
                             ? std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{timeout})
                             : fork_api_impl_::DEFAULT_TIMEOUT_NS};
  auto result_opt{co_await wait_with_timeout_t{queue.val()->pop(), timeout_ns}};
  co_return result_opt.has_value() ? result_opt.value() : Optional<int64_t>{};
}
