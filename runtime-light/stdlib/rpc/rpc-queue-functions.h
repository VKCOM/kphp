// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/fork/fork-functions.h"
#include "runtime-light/stdlib/rpc/rpc-client-state.h"
#include "runtime-light/stdlib/rpc/rpc-queue-state.h"

namespace kphp::rpc {

inline void rpc_queue_push(int64_t queue_id, int64_t request_id) noexcept {
  static constexpr auto rpc_queue_wrapper_task{[](kphp::coro::shared_task<> awaiter_task, int64_t request_id) noexcept -> kphp::coro::task<int64_t> {
    co_await std::move(awaiter_task).when_ready();
    co_return request_id;
  }};

  auto& rpc_client_instance_st{RpcClientInstanceState::get()};

  const auto it_awaiter_task{rpc_client_instance_st.response_awaiter_tasks.find(request_id)};
  if (it_awaiter_task == rpc_client_instance_st.response_awaiter_tasks.end()) [[unlikely]] {
    kphp::log::warning("could not find rpc query with id {} in pending queries", queue_id);
    return;
  }

  auto& rpc_queue_instance_st{RpcQueueInstanceState::get()};
  auto opt_await_set{rpc_queue_instance_st.get_queue(queue_id)};
  if (!opt_await_set) [[unlikely]] {
    kphp::log::warning("future with id {} isn't associated with rpc queue", queue_id);
    return;
  }

  auto& await_set{(*opt_await_set).get()};
  await_set.push(rpc_queue_wrapper_task(static_cast<kphp::coro::shared_task<>>(it_awaiter_task->second), request_id));
}

inline int64_t rpc_queue_create(std::span<int64_t> request_ids) noexcept {
  const int64_t queue_id{RpcQueueInstanceState::get().create_queue()};
  for (int64_t request_id : request_ids) {
    rpc_queue_push(queue_id, request_id);
  }

  return queue_id;
}

inline kphp::coro::task<std::optional<int64_t>> rpc_queue_next(int64_t queue_id, std::chrono::nanoseconds timeout) noexcept {
  static constexpr auto rpc_queue_next_task{
      [](auto await_set_awaitable) noexcept -> kphp::coro::task<std::optional<int64_t>> { co_return co_await std::move(await_set_awaitable); }};

  auto& rpc_queue_instance_st{RpcQueueInstanceState::get()};
  auto opt_await_set{rpc_queue_instance_st.get_queue(queue_id)};

  if (!opt_await_set) [[unlikely]] {
    kphp::log::warning("future with id {} isn't associated with rpc queue", queue_id);
    co_return std::nullopt;
  }

  auto& await_set{(*opt_await_set).get()};
  const auto expected_next{co_await kphp::coro::io_scheduler::get().schedule(rpc_queue_next_task(await_set.next()), timeout)};
  if (!expected_next) {
    co_return std::nullopt;
  }

  const auto opt_request_id{*expected_next};
  if (!opt_request_id) [[unlikely]] {
    kphp::log::warning("failed to get RPC response from rpc queue {}", queue_id);
    co_return kphp::rpc::INVALID_QUERY_ID;
  }
  co_return *opt_request_id;
}

inline bool rpc_queue_empty(int64_t queue_id) noexcept {
  auto& rpc_queue_instance_st{RpcQueueInstanceState::get()};
  auto opt_await_set{rpc_queue_instance_st.get_queue(queue_id)};
  if (!opt_await_set) [[unlikely]] {
    kphp::log::warning("future with id {} isn't associated with rpc queue", queue_id);
    return true;
  }
  const auto& await_set{(*opt_await_set).get()};
  return await_set.empty();
}

} // namespace kphp::rpc

inline int64_t f$rpc_queue_create() noexcept {
  return kphp::rpc::rpc_queue_create({});
}

inline int64_t f$rpc_queue_create(const mixed& request_ids) noexcept {
  const int64_t queue_id{kphp::rpc::rpc_queue_create({})};
  if (!request_ids.is_array()) {
    return queue_id;
  }
  for (auto request_id : request_ids.as_array()) {
    if (request_id.get_value().is_int()) {
      kphp::rpc::rpc_queue_push(queue_id, request_id.get_value().as_int());
    }
  }

  return queue_id;
}

inline void f$rpc_queue_push(int64_t queue_id, int64_t request_id) noexcept {
  return kphp::rpc::rpc_queue_push(queue_id, request_id);
}

inline bool f$rpc_queue_empty(int64_t queue_id) noexcept {
  return kphp::rpc::rpc_queue_empty(queue_id);
}

inline kphp::coro::task<Optional<int64_t>> f$rpc_queue_next(int64_t queue_id, double timeout = -1) noexcept {
  auto opt_result{co_await kphp::forks::id_managed(
      kphp::rpc::rpc_queue_next(queue_id, std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{timeout})))};
  co_return opt_result ? *opt_result : Optional<int64_t>{};
}
