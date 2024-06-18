// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/coroutine/fork-context.h"

template<typename T, bool is_php_runtime_type = true>
task_t<T> f$wait(int64_t fork_id, double timeout = -1.0) {
  co_return co_await wait_fork_t<internal_optional_type<T>>{fork_id, timeout};
}

template<typename T>
task_t<T> f$wait(Optional<int64_t> resumable_id, double timeout = -1.0) {
  co_return co_await f$wait<internal_optional_type<T>>(resumable_id.val(), timeout);
}

inline task_t<void> f$sched_yield() {
  co_return co_await sched_yield_t{};
}

inline task_t<void> f$sched_yield_sleep(int64_t timeout_ns) {
  co_return co_await sched_yield_sleep_t{timeout_ns};
}
