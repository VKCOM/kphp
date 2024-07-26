// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

#include "runtime-core/core-types/decl/optional.h"
#include "runtime-core/utils/kphp-assert-core.h"
#include "runtime-light/component/component.h"
#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/fork/fork-context.h"

template<typename T>
requires(is_optional<T>::value) task_t<T> f$wait(Optional<int64_t> fork_id, double timeout = -1.0) noexcept {
  constexpr double WAIT_FORK_MAX_TIMEOUT = 86400.0;
  auto task_opt{ForkComponentContext::get().pop_fork(fork_id.has_value() ? fork_id.val() : INVALID_PLATFORM_DESCRIPTOR)};
  if (!task_opt.has_value()) {
    php_warning("can't find fork %" PRId64, fork_id.val());
    co_return T{};
  }

  if (timeout < 0.0) {
    timeout = WAIT_FORK_MAX_TIMEOUT;
  }
  const auto timeout_ns{std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{timeout})};
  co_return co_await wait_fork_t<internal_optional_type_t<T>>{*std::move(task_opt), timeout_ns};
}
