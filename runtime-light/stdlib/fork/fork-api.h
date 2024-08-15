// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <cstdint>

#include "runtime-core/core-types/decl/optional.h"
#include "runtime-core/utils/kphp-assert-core.h"
#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/fork/fork-context.h"
#include "runtime-light/stdlib/fork/wait-queue-context.h"

namespace fork_api_impl_ {

constexpr double MAX_TIMEOUT_S = 86400.0;
constexpr double DEFAULT_TIMEOUT_S = MAX_TIMEOUT_S;
constexpr auto MAX_TIMEOUT_NS = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{MAX_TIMEOUT_S});
constexpr auto DEFAULT_TIMEOUT_NS = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{DEFAULT_TIMEOUT_S});

} // namespace fork_api_impl_

template<typename T>
requires(is_optional<T>::value) task_t<T> f$wait(int64_t fork_id, double timeout = -1.0) noexcept {
  auto &fork_ctx{ForkComponentContext::get()};
  if (!fork_ctx.contains(fork_id)) {
    php_warning("can't find fork %" PRId64, fork_id);
    co_return T{};
  }
  // normalize timeout
  const auto timeout_ns{timeout > 0 && timeout <= fork_api_impl_::MAX_TIMEOUT_S
                          ? std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{timeout})
                          : fork_api_impl_::DEFAULT_TIMEOUT_NS};
  auto result_opt{co_await wait_with_timeout_t{wait_fork_t<internal_optional_type_t<T>>{fork_id}, timeout_ns}};
  co_return result_opt.has_value() ? T{std::move(result_opt.value())} : T{};
}

template<typename T>
requires(is_optional<T>::value) task_t<T> f$wait(Optional<int64_t> fork_id_opt, double timeout = -1.0) noexcept {
  co_return co_await f$wait<T>(fork_id_opt.has_value() ? fork_id_opt.val() : INVALID_FORK_ID, timeout);
}

task_t<void> f$sched_yield() noexcept;

task_t<void> f$sched_yield_sleep(int64_t duration_ns) noexcept;

inline int64_t f$wait_queue_create(array<Optional<int64_t>> fork_ids) noexcept {
  return WaitQueueContext::get().create_queue(fork_ids);
}

inline void f$wait_queue_push(int64_t queue_id, Optional<int64_t> fork_id) noexcept {
  if (auto queue = WaitQueueContext::get().get_queue(queue_id); queue.has_value() && fork_id.has_value()) {
    queue.val()->push(fork_id.val());
  }
}

inline bool f$wait_queue_empty(int64_t queue_id) noexcept {
  if (auto queue = WaitQueueContext::get().get_queue(queue_id); queue.has_value()) {
    return queue.val()->empty();
  }
  return true;
}

task_t<Optional<int64_t>> f$wait_queue_next(int64_t queue, double timeout = -1.0) noexcept;
