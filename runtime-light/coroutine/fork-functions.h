#pragma once

#include "runtime-light/coroutine/fork.h"
#include "runtime-light/coroutine/awaitable.h"

template<typename T, bool is_php_runtime_type = true>
task_t<T> f$wait(int64_t fork_id) {
  co_return co_await wait_fork_t<internal_optional_type<T>>{fork_id};
}

template<typename T>
task_t<T> f$wait(Optional<int64_t> resumable_id) {
  co_return co_await f$wait<internal_optional_type<T>>(resumable_id.val());
}

inline task_t<void> f$sched_yield() {
  co_return co_await sched_yield_t{};
}
