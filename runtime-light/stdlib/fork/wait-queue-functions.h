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
#include "runtime-light/stdlib/fork/future-queue.h"
#include "runtime-light/stdlib/fork/wait-queue-state.h"

namespace kphp::forks {

inline kphp::coro::task<std::optional<int64_t>> wait_queue_next(int64_t queue_id, std::chrono::nanoseconds timeout) noexcept {
  auto& wait_queue_instance_st{WaitQueueInstanceState::get()};
  auto opt_await_set{wait_queue_instance_st.get_queue(queue_id)};
  if (!opt_await_set.has_value()) [[unlikely]] {
    kphp::log::warning("future with id {} doesn't associated with wait queue", queue_id);
    co_return std::nullopt;
  }

  auto& await_set{(*opt_await_set).get()};
  // SAFETY: await_set is stored in WaitQueueInstanceState. If it is removed, it will wake up the coroutine
  static constexpr auto wait_queue_next_task{
      [](auto await_set_awaitable) noexcept -> kphp::coro::task<std::optional<int64_t>> { co_return co_await std::move(await_set_awaitable); }};
  auto wait_result{co_await kphp::coro::io_scheduler::get().schedule(wait_queue_next_task(await_set.next()), kphp::forks::detail::normalize_timeout(timeout))};
  if (!wait_result) {
    co_return std::nullopt;
  }

  if (auto opt_future{*wait_result}; opt_future.has_value()) {
    auto opt_info{ForkInstanceState::get().get_info(*opt_future)};
    kphp::log::assertion(opt_info.has_value());
    auto& fork_info{(*opt_info)};
    fork_info.get().awaited = false; // Open access for awaiting the future. See the comment for wait_queue_push.
    co_return *opt_future;
  } else {
    kphp::log::warning("await set associated with wait queue was destroyed");
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
    kphp::log::warning("future with id {} doesn't associated with wait queue", queue_id);
    return;
  }

  auto fork_info{(*opt_info)};
  auto fork_task{*fork_info.get().opt_handle};
  fork_info.get().awaited = true; // future that was pushed in the wait-queue cannot be waited until it is ready (Until it returns from wait_queue_next)

  static constexpr auto wait_queue_wrapper_task{[](kphp::coro::shared_task<> fork_task, int64_t fork_id) noexcept -> kphp::coro::task<int64_t> {
    co_await std::move(fork_task).when_ready();
    co_return fork_id;
  }};

  auto& await_set{(*opt_await_set).get()};
  await_set.push(wait_queue_wrapper_task(std::move(fork_task), fork_id));
}

} // namespace kphp::forks

template<typename T>
void f$wait_queue_push(kphp::forks::wait_queue_future<T> future, Optional<int64_t> fork_id) noexcept {
  if (!fork_id.has_value()) {
    return;
  }
  kphp::forks::wait_queue_push(future.m_future_id, fork_id.val());
}

template<typename T>
bool f$wait_queue_empty(kphp::forks::wait_queue_future<T> future) noexcept {
  auto& wait_queue_instance_st{WaitQueueInstanceState::get()};
  auto opt_await_set{wait_queue_instance_st.get_queue(future.m_future_id)};
  if (!opt_await_set.has_value()) [[unlikely]] {
    kphp::log::warning("future with id {} doesn't associated with wait queue", future.m_future_id);
    return true;
  }
  const auto& await_set{(*opt_await_set).get()};
  return await_set.empty();
}

template<typename T>
kphp::coro::task<Optional<future<T>>> f$wait_queue_next(kphp::forks::wait_queue_future<T> future, double timeout = -1.0) noexcept {
  auto opt_result{co_await kphp::forks::id_managed(
      kphp::forks::wait_queue_next(future.m_future_id, std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{timeout})))};
  co_return opt_result ? *std::move(opt_result) : Optional<int64_t>{};
}

template<typename future_queue_type>
requires(kphp::forks::is_wait_queue_future_v<future_queue_type>)
future_queue_type f$wait_queue_create() noexcept {
  auto& wait_queue_context{WaitQueueInstanceState::get()};
  const int64_t queue_id{wait_queue_context.create_queue()};
  return future_queue_type{queue_id};
}

template<typename future_queue_type>
requires(kphp::forks::is_wait_queue_future_v<future_queue_type>)
future_queue_type f$wait_queue_create(const mixed& fork_ids) noexcept {
  auto& wait_queue_context{WaitQueueInstanceState::get()};
  const int64_t queue_id{wait_queue_context.create_queue()};
  auto future{future_queue_type{queue_id}};
  if (!fork_ids.is_array()) {
    return future;
  }
  for (auto fork_id : fork_ids.as_array()) {
    if (fork_id.get_value().is_int()) {
      kphp::forks::wait_queue_push(future.m_future_id, fork_id.get_value().as_int());
    }
  }

  return future;
}
