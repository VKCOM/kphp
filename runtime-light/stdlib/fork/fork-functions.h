// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <concepts>
#include <cstdint>
#include <utility>

#include "runtime-common/core/core-types/decl/optional.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/coroutine/shared_task.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/fork/fork-state.h"

namespace forks_impl_ {

inline constexpr double MAX_TIMEOUT_S = 86400.0;
inline constexpr double DEFAULT_TIMEOUT_S = MAX_TIMEOUT_S;
inline constexpr auto MAX_TIMEOUT_NS = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{MAX_TIMEOUT_S});
inline constexpr auto DEFAULT_TIMEOUT_NS = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{DEFAULT_TIMEOUT_S});

inline std::chrono::nanoseconds normalize_timeout(double timeout_s) noexcept {
  if (timeout_s < 0 || timeout_s > MAX_TIMEOUT_S) [[unlikely]] {
    return DEFAULT_TIMEOUT_NS;
  }
  return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{timeout_s});
}

} // namespace forks_impl_

template<typename T>
requires(is_optional<T>::value || std::same_as<T, mixed> || is_class_instance<T>::value) task_t<T> f$wait(int64_t fork_id, double timeout = -1.0) noexcept {
  auto &fork_instance_st{ForkInstanceState::get()};
  auto opt_fork_task{fork_instance_st.get_fork(fork_id)};
  if (!opt_fork_task.has_value()) [[unlikely]] {
    php_warning("fork with ID %" PRId64 " does not exist or has already been awaited by another fork", fork_id);
    co_return T{};
  }

  auto fork_task{static_cast<shared_task_t<internal_optional_type_t<T>>>(*std::move(opt_fork_task))};
  auto opt_result{co_await wait_with_timeout_t{fork_task.operator co_await(), forks_impl_::normalize_timeout(timeout)}};
  // mark the fork as awaited and remove it from the set of active forks
  fork_instance_st.erase_fork(fork_id);
  co_return opt_result.has_value() ? T{std::move(opt_result.value())} : T{};
}

template<typename T>
requires(is_optional<T>::value || std::same_as<T, mixed> || is_class_instance<T>::value) task_t<T> f$wait(Optional<int64_t> opt_fork_id,
                                                                                                          double timeout = -1.0) noexcept {
  co_return co_await f$wait<T>(opt_fork_id.has_value() ? opt_fork_id.val() : kphp::forks::INVALID_ID, timeout);
}

// ================================================================================================

inline task_t<bool> f$wait_concurrently(int64_t fork_id) noexcept {
  auto &fork_instance_st{ForkInstanceState::get()};
  auto it{fork_instance_st.forks.find(fork_id)};
  if (it == fork_instance_st.forks.end()) [[unlikely]] {
    co_return false;
  }

  if (it->second.first != ForkInstanceState::fork_state::awaited) [[likely]] {
    auto fork_task{it->second.second};
    co_await fork_task.when_ready();
  }
  co_return true;
}

inline task_t<bool> f$wait_concurrently(Optional<int64_t> opt_fork_id) noexcept {
  co_return co_await f$wait_concurrently(opt_fork_id.has_value() ? opt_fork_id.val() : kphp::forks::INVALID_ID);
}

inline task_t<bool> f$wait_concurrently(const mixed &fork_id) noexcept {
  co_return co_await f$wait_concurrently(fork_id.to_int());
}

// ================================================================================================

inline task_t<void> f$sched_yield() noexcept {
  co_await wait_for_reschedule_t{};
}

inline task_t<void> f$sched_yield_sleep(double duration) noexcept {
  if (duration <= 0) {
    php_warning("can't sleep for negative or zero duration %.9f", duration);
    co_return;
  }
  co_await wait_for_timer_t{std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{duration})};
}

// ================================================================================================

inline int64_t f$get_running_fork_id() noexcept {
  return ForkInstanceState::get().current_id;
}

inline int64_t f$wait_queue_create() {
  php_critical_error("call to unsupported function");
}

inline int64_t f$wait_queue_create(const mixed & /*resumable_ids*/) {
  php_critical_error("call to unsupported function");
}

inline int64_t f$wait_queue_push(int64_t /*queue_id*/, const mixed & /*resumable_ids*/) {
  php_critical_error("call to unsupported function");
}

inline bool f$wait_queue_empty(int64_t /*queue_id*/) {
  php_critical_error("call to unsupported function");
}

inline Optional<int64_t> f$wait_queue_next(int64_t /*queue_id*/, double /*timeout*/ = -1.0) {
  php_critical_error("call to unsupported function");
}

template<typename T>
T f$wait_multi(const array<Optional<int64_t>> & /*resumable_ids*/) {
  php_critical_error("call to unsupported function");
}

template<typename T>
T f$wait_multi(const array<int64_t> & /*resumable_ids*/) {
  php_critical_error("call to unsupported function");
}
