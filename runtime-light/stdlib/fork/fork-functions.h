// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

#include "runtime-common/core/core-types/decl/optional.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/concepts.h"
#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/coroutine/shared-task.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/coroutine/type-traits.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/fork/fork-state.h"

namespace kphp::forks {

namespace detail {

inline constexpr double MAX_TIMEOUT = 86400.0;
inline constexpr double DEFAULT_TIMEOUT = MAX_TIMEOUT;

inline constexpr auto MAX_TIMEOUT_NS = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{MAX_TIMEOUT});
inline constexpr auto DEFAULT_TIMEOUT_NS = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{DEFAULT_TIMEOUT});

inline std::chrono::nanoseconds normalize_timeout(std::chrono::nanoseconds timeout) noexcept {
  using namespace std::chrono_literals;
  if (timeout <= 0ns || timeout > MAX_TIMEOUT_NS) {
    return DEFAULT_TIMEOUT_NS;
  }
  return timeout;
}

} // namespace detail

template<kphp::coro::concepts::awaitable awaitable_type>
auto id_managed(awaitable_type awaitable) noexcept -> kphp::coro::task<typename kphp::coro::awaitable_traits<awaitable_type>::awaiter_return_type> {
  auto& fork_instance_st{ForkInstanceState::get()};
  const auto saved_fork_id{fork_instance_st.current_id};
  if constexpr (std::is_void_v<typename kphp::coro::awaitable_traits<awaitable_type>::awaiter_return_type>) {
    co_await std::move(awaitable);
    fork_instance_st.current_id = saved_fork_id;
    co_return;
  } else {
    auto value{co_await std::move(awaitable)};
    fork_instance_st.current_id = saved_fork_id;
    co_return std::move(value);
  }
}

template<typename return_type>
auto start(kphp::coro::task<return_type> task) noexcept -> int64_t {
  auto& fork_instance_st{ForkInstanceState::get()};
  auto [fork_id, fork_task]{fork_instance_st.create_fork(std::move(task))};
  auto saved_fork_id{fork_instance_st.current_id};
  kphp::log::assertion(kphp::coro::io_scheduler::get().start(std::move(fork_task)));
  fork_instance_st.current_id = saved_fork_id;
  return fork_id;
}

template<typename return_type>
auto wait(int64_t fork_id, std::chrono::nanoseconds timeout) noexcept -> kphp::coro::task<std::optional<return_type>> {
  auto& fork_instance_st{ForkInstanceState::get()};
  auto opt_info{fork_instance_st.get_info(fork_id)};
  if (!opt_info || !(*opt_info).get().opt_handle || (*opt_info).get().awaited) [[unlikely]] {
    kphp::log::warning("fork does not exist or has already been awaited by another fork: id -> {}", fork_id);
    co_return std::nullopt;
  }

  auto fork_info{(*opt_info)};
  auto fork_task{*fork_info.get().opt_handle};
  fork_info.get().awaited = true; // prevent further f$wait from awaiting on the same fork

  // Important: capture current fork's info pre-co_await.
  // Fork ID is not automatically preserved across suspension points
  auto current_fork_info{fork_instance_st.current_info()};
  auto expected{co_await kphp::coro::io_scheduler::get().schedule(static_cast<kphp::coro::shared_task<return_type>>(std::move(fork_task)),
                                                                  detail::normalize_timeout(timeout))};

  // Execute essential housekeeping tasks to maintain proper state management.
  // 1. Check for any exceptions that may have occurred during the fork execution. If an exception is found, propagate it to the current fork.
  //    Clean fork_info's exception state.
  kphp::log::assertion(std::exchange(current_fork_info.get().thrown_exception, std::move(fork_info.get().thrown_exception)).is_null());
  // 2. Detach the shared_task from fork_info to prevent further associations, ensuring that resources are released.
  fork_info.get().opt_handle.reset();

  if (!expected) [[unlikely]] {
    co_return std::nullopt;
  }
  co_return *std::move(expected);
}

} // namespace kphp::forks

template<typename T>
requires(is_optional<T>::value || std::same_as<T, mixed> || is_class_instance<T>::value)
kphp::coro::task<T> f$wait(int64_t fork_id, double timeout = -1.0) noexcept {
  auto opt_result{co_await kphp::forks::id_managed(
      kphp::forks::wait<internal_optional_type_t<T>>(fork_id, std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{timeout})))};
  co_return opt_result ? T{*std::move(opt_result)} : T{};
}

template<typename T>
requires(is_optional<T>::value || std::same_as<T, mixed> || is_class_instance<T>::value)
kphp::coro::task<T> f$wait(Optional<int64_t> opt_fork_id, double timeout = -1.0) noexcept {
  co_return co_await f$wait<T>(opt_fork_id.has_value() ? opt_fork_id.val() : kphp::forks::INVALID_ID, timeout);
}

// ================================================================================================

inline kphp::coro::task<bool> f$wait_concurrently(int64_t fork_id) noexcept {
  auto& fork_instance_st{ForkInstanceState::get()};
  auto opt_info{fork_instance_st.get_info(fork_id)};
  if (!opt_info) [[unlikely]] {
    co_return false;
  }

  const auto fork_info{*opt_info};
  if (fork_info.get().opt_handle) {
    auto fork_task{*fork_info.get().opt_handle};
    co_await kphp::forks::id_managed(fork_task.when_ready());
  }
  co_return true;
}

inline kphp::coro::task<bool> f$wait_concurrently(Optional<int64_t> opt_fork_id) noexcept {
  co_return co_await f$wait_concurrently(opt_fork_id.has_value() ? opt_fork_id.val() : kphp::forks::INVALID_ID);
}

inline kphp::coro::task<bool> f$wait_concurrently(const mixed& fork_id) noexcept {
  co_return co_await f$wait_concurrently(fork_id.to_int());
}

template<typename T>
kphp::coro::task<T> f$wait_multi(array<int64_t> fork_ids) noexcept {
  T res{};
  for (const auto& it : std::as_const(fork_ids)) {
    res.set_value(it.get_key(), co_await f$wait<typename T::value_type>(it.get_value()));
  }
  co_return std::move(res);
}

template<typename T>
kphp::coro::task<T> f$wait_multi(array<Optional<int64_t>> fork_ids) noexcept {
  const auto ids{array<int64_t>::convert_from(fork_ids)};
  co_return co_await f$wait_multi<T>(ids);
}

// ================================================================================================

inline kphp::coro::task<> f$sched_yield() noexcept {
  co_await kphp::forks::id_managed(kphp::coro::io_scheduler::get().yield());
}

inline kphp::coro::task<> f$sched_yield_sleep(double duration) noexcept {
  const auto timeout{std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{duration})};
  co_await kphp::forks::id_managed(kphp::coro::io_scheduler::get().yield_for(kphp::forks::detail::normalize_timeout(timeout)));
}

// ================================================================================================

inline int64_t f$get_running_fork_id() noexcept {
  return ForkInstanceState::get().current_id;
}

inline int64_t f$wait_queue_create() {
  kphp::log::info("called stub wait_queue_create()");
  return 0;
}

inline int64_t f$wait_queue_create(const mixed& /*resumable_ids*/) {
  kphp::log::info("called stub wait_queue_create(ids)");
  return 0;
}

inline int64_t f$wait_queue_push(int64_t /*queue_id*/, const mixed& /*resumable_ids*/) {
  kphp::log::error("call to unsupported function");
}

inline bool f$wait_queue_empty(int64_t /*queue_id*/) {
  kphp::log::error("call to unsupported function");
}

inline Optional<int64_t> f$wait_queue_next(int64_t /*queue_id*/, double /*timeout*/ = -1.0) {
  kphp::log::error("call to unsupported function");
}
