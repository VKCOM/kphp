// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <cinttypes>
#include <concepts>
#include <cstdint>
#include <optional>
#include <utility>

#include "runtime-common/core/core-types/decl/optional.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/coroutine/shared-task.h"
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
  auto opt_fork_info{fork_instance_st.get_info(fork_id)};
  if (!opt_fork_info.has_value() || !(*opt_fork_info).get().opt_handle.has_value() || (*opt_fork_info).get().awaited) [[unlikely]] {
    php_warning("fork with ID %" PRId64 " does not exist or has already been awaited by another fork", fork_id);
    co_return T{};
  }

  auto &fork_info{(*opt_fork_info).get()};
  auto fork_task{*fork_info.opt_handle};
  fork_info.awaited = true; // prevent further f$wait from awaiting on the same fork
  auto opt_result{co_await wait_with_timeout_t{wait_fork_t{static_cast<shared_task_t<internal_optional_type_t<T>>>(std::move(fork_task))},
                                               forks_impl_::normalize_timeout(timeout)}};
  // Execute essential housekeeping tasks to maintain proper state management.
  // 1. Check for any exceptions that may have occurred during the fork execution. If an exception is found, propagate it to the current fork.
  //    Clean fork_info's exception state.
  auto current_fork_info{fork_instance_st.current_info()};
  php_assert(std::exchange(current_fork_info.get().thrown_exception, std::move(fork_info.thrown_exception)).is_null());
  // 2. Detach the shared_task_t from fork_info to prevent further associations, ensuring that resources are released.
  fork_info.opt_handle.reset();

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
  auto opt_fork_info{fork_instance_st.get_info(fork_id)};
  if (!opt_fork_info.has_value()) [[unlikely]] {
    co_return false;
  }

  const auto &fork_info{(*opt_fork_info).get()};
  if (fork_info.opt_handle.has_value()) {
    auto fork_task{*fork_info.opt_handle};
    co_await wait_fork_t{std::move(fork_task)};
  }
  co_return true;
}

inline task_t<bool> f$wait_concurrently(Optional<int64_t> opt_fork_id) noexcept {
  co_return co_await f$wait_concurrently(opt_fork_id.has_value() ? opt_fork_id.val() : kphp::forks::INVALID_ID);
}

inline task_t<bool> f$wait_concurrently(const mixed &fork_id) noexcept {
  co_return co_await f$wait_concurrently(fork_id.to_int());
}

template<typename T>
task_t<T> f$wait_multi(array<int64_t> fork_ids) noexcept {
  T res{};
  for (const auto &it : fork_ids) {
    res.set_value(it.get_key(), co_await f$wait<typename T::value_type>(it.get_value()));
  }
  co_return std::move(res);
}

template<typename T>
task_t<T> f$wait_multi(array<Optional<int64_t>> fork_ids) noexcept {
  const auto ids{array<int64_t>::convert_from(fork_ids)};
  co_return co_await f$wait_multi<T>(ids);
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
