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
    co_return internal_optional_type_t<T>{};
  }
  // normalize timeout
  const auto timeout_ns{timeout > 0 && timeout <= fork_api_impl_::MAX_TIMEOUT_S
                          ? std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{timeout})
                          : fork_api_impl_::DEFAULT_TIMEOUT_NS};
  co_return(co_await wait_with_timeout_t{wait_fork_t<internal_optional_type_t<T>>{fork_id}, timeout_ns}).value_or(internal_optional_type_t<T>{});
}

template<typename T>
requires(is_optional<T>::value) task_t<T> f$wait(Optional<int64_t> fork_id_opt, double timeout = -1.0) noexcept {
  co_return co_await f$wait<T>(fork_id_opt.has_value() ? fork_id_opt.val() : INVALID_FORK_ID, timeout);
}

task_t<void> f$sched_yield() noexcept;

task_t<void> f$sched_yield_sleep(int64_t duration_ns) noexcept;
