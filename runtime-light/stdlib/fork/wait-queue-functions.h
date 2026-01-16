// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

#include "runtime-common/core/core-types/decl/optional.h"
#include "runtime-light/coroutine/await-set.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/fork/fork-functions.h"
#include "runtime-light/stdlib/fork/fork-state.h"
#include "runtime-light/stdlib/fork/fork-storage.h"
#include "runtime-light/stdlib/fork/wait-queue-state.h"

namespace kphp::forks {

inline kphp::coro::task<std::optional<int64_t>> wait_queue_next(int64_t queue_id, std::chrono::nanoseconds timeout) noexcept {
  auto& wait_queue_instance_st{WaitQueueInstanceState::get()};
  auto opt_await_set{wait_queue_instance_st.get_queue(queue_id)};
  if (!opt_await_set.has_value()) [[unlikely]] {
    kphp::log::warning("future with id {} isn't associated with wait queue", queue_id);
    co_return std::nullopt;
  }

  auto& await_set{(*opt_await_set).get()};
  static constexpr auto wait_queue_next_task{
      [](auto await_set_awaitable) noexcept -> kphp::coro::task<std::optional<int64_t>> { co_return co_await std::move(await_set_awaitable); }};
  auto wait_result{co_await kphp::coro::io_scheduler::get().schedule(wait_queue_next_task(await_set.next()), kphp::forks::detail::normalize_timeout(timeout))};
  if (!wait_result) {
    co_return std::nullopt;
  }

  if (auto opt_future{*wait_result}; opt_future.has_value()) {
    auto opt_info{ForkInstanceState::get().get_info(*opt_future)};
    kphp::log::assertion(opt_info.has_value());
    auto fork_info{(*opt_info)};
    fork_info.get().awaited = false; // Open access for awaiting the future. See the comment for wait_queue_push.
    co_return *opt_future;
  } else {
    kphp::log::warning("await set associated with the wait queue was destroyed");
    co_return kphp::forks::INVALID_ID;
  }
}

inline void wait_queue_push(int64_t queue_id, int64_t fork_id) noexcept {
  auto& fork_instance_st{ForkInstanceState::get()};
  auto opt_info{fork_instance_st.get_info(fork_id)};
  if (!opt_info || !(*opt_info).get().opt_handle || (*opt_info).get().awaited) [[unlikely]] {
    kphp::log::warning("fork does not exist or has already been awaited by another fork: id -> {}", fork_id);
    return;
  }

  auto& wait_queue_instance_st{WaitQueueInstanceState::get()};
  auto opt_await_set{wait_queue_instance_st.get_queue(queue_id)};
  if (!opt_await_set.has_value()) [[unlikely]] {
    kphp::log::warning("future with id {} isn't associated with wait queue", queue_id);
    return;
  }

  auto fork_info{(*opt_info)};
  auto fork_task{*fork_info.get().opt_handle};
  fork_info.get().awaited = true; // future that was pushed in the wait-queue cannot be waited until it is ready (Until it returns from wait_queue_next)

  static constexpr auto wait_queue_wrapper_task{
      [](kphp::coro::shared_task<kphp::forks::details::storage> fork_task, int64_t fork_id) noexcept -> kphp::coro::task<int64_t> {
        co_await std::move(fork_task).when_ready();
        co_return fork_id;
      }};

  auto& await_set{(*opt_await_set).get()};
  await_set.push(wait_queue_wrapper_task(std::move(fork_task), fork_id));
}

} // namespace kphp::forks

inline void f$wait_queue_push(int64_t queue_id, Optional<int64_t> fork_id) noexcept {
  if (!fork_id.has_value()) {
    return;
  }
  kphp::forks::wait_queue_push(queue_id, fork_id.val());
}

inline bool f$wait_queue_empty(int64_t queue_id) noexcept {
  auto& wait_queue_instance_st{WaitQueueInstanceState::get()};
  auto opt_await_set{wait_queue_instance_st.get_queue(queue_id)};
  if (!opt_await_set.has_value()) [[unlikely]] {
    kphp::log::warning("future with id {} isn't associated with wait queue", queue_id);
    return true;
  }
  const auto& await_set{(*opt_await_set).get()};
  return await_set.empty();
}

inline kphp::coro::task<Optional<int64_t>> f$wait_queue_next(int64_t queue_id, double timeout = -1.0) noexcept {
  auto opt_result{co_await kphp::forks::id_managed(
      kphp::forks::wait_queue_next(queue_id, std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{timeout})))};
  co_return opt_result ? *opt_result : Optional<int64_t>{};
}

inline int64_t f$wait_queue_create() noexcept {
  auto& wait_queue_context{WaitQueueInstanceState::get()};
  const int64_t queue_id{wait_queue_context.create_queue()};
  return queue_id;
}

inline int64_t f$wait_queue_create(const mixed& fork_ids) noexcept {
  auto& wait_queue_context{WaitQueueInstanceState::get()};
  const int64_t queue_id{wait_queue_context.create_queue()};
  auto future{queue_id};
  if (!fork_ids.is_array()) {
    return future;
  }
  for (auto fork_id : fork_ids.as_array()) {
    if (fork_id.get_value().is_int()) {
      kphp::forks::wait_queue_push(future, fork_id.get_value().as_int());
    }
  }

  return future;
}
