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

int64_t f$wait_queue_create(array<Optional<int64_t>> fork_ids) noexcept {
  return WaitQueueContext::get().create_queue(fork_ids);
}

void f$wait_queue_push(int64_t queue_id, Optional<int64_t> fork_id) noexcept {
  if (auto queue = WaitQueueContext::get().get_queue(queue_id); queue.has_value() && fork_id.has_value()) {
    auto task = ForkComponentContext::get().pop_fork(fork_id.val());
    if (task.has_value()) {
      php_debug("push fork %ld, to queue %ld", fork_id.val(), queue_id);
      queue.val()->push_fork(fork_id.val(), std::move(task.val()));
    }
  }
}

bool f$wait_queue_empty(int64_t queue_id) noexcept {
  if (auto queue = WaitQueueContext::get().get_queue(queue_id); queue.has_value()) {
    return queue.val()->empty();
  }
  return false;
}

task_t<Optional<int64_t>> f$wait_queue_next(int64_t queue_id, double timeout) noexcept {
  if (WaitQueueContext::get().get_queue(queue_id).has_value()) {
    if (timeout < 0.0) {
      timeout = fork_api_impl_::WAIT_FORK_MAX_TIMEOUT;
    }
    const auto timeout_ns{std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{timeout})};
    co_return co_await wait_queue_next_t{queue_id, timeout_ns};
  }
  co_return Optional<int64_t>{};
}
