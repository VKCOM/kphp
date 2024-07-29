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

namespace fork_api_impl_ {

constexpr double WAIT_FORK_MAX_TIMEOUT = 86400.0;

} // namespace fork_api_impl_

constexpr int64_t INVALID_FORK_ID = -1;

template<typename T>
requires(is_optional<T>::value) task_t<T> f$wait(int64_t fork_id, double timeout = -1.0) noexcept {
  if (timeout < 0.0) {
    timeout = fork_api_impl_::WAIT_FORK_MAX_TIMEOUT;
  }
  auto task_opt{ForkComponentContext::get().pop_fork(fork_id)};
  if (!task_opt.has_value()) {
    php_warning("can't find fork %" PRId64, fork_id);
    co_return T{};
  }
  const auto timeout_ns{std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{timeout})};
  co_return co_await wait_fork_t<internal_optional_type_t<T>>{*std::move(task_opt), timeout_ns};
}

template<typename T>
requires(is_optional<T>::value) task_t<T> f$wait(Optional<int64_t> fork_id_opt, double timeout = -1.0) noexcept {
  co_return co_await f$wait<T>(fork_id_opt.has_value() ? fork_id_opt.val() : INVALID_FORK_ID, timeout);
}

task_t<void> f$sched_yield() noexcept;

task_t<void> f$sched_yield_sleep(int64_t duration_ns) noexcept;
