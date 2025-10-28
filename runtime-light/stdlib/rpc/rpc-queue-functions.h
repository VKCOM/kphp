// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/fork/fork-functions.h"
#include "runtime-light/stdlib/fork/wait-queue-functions.h"
#include "runtime-light/stdlib/rpc/rpc-client-state.h"

namespace kphp::rpc {

inline void rpc_queue_push(int64_t queue_id, int64_t request_id) noexcept {
  auto& rpc_client_instance_st{RpcClientInstanceState::get()};

  const auto it_fork_id{rpc_client_instance_st.response_waiter_forks.find(request_id)};
  if (it_fork_id == rpc_client_instance_st.response_waiter_forks.end()) [[unlikely]] {
    kphp::log::warning("unexpectedly could not find query with id {} in pending queries", queue_id);
    return;
  }

  const int64_t response_waiter_fork_id{it_fork_id->second};
  rpc_client_instance_st.awaiter_forks_to_response.emplace(response_waiter_fork_id, request_id);
  kphp::forks::wait_queue_push(queue_id, response_waiter_fork_id);
}

inline kphp::coro::task<std::optional<int64_t>> rpc_queue_next(int64_t queue_id, std::chrono::nanoseconds timeout) noexcept {
  auto wait_result{co_await kphp::forks::wait_queue_next(queue_id, timeout)};
  if (!wait_result.has_value()) {
    co_return std::nullopt;
  }

  auto& rpc_client_instance_st{RpcClientInstanceState::get()};
  const auto it_request_id{rpc_client_instance_st.awaiter_forks_to_response.find(*wait_result)};
  if (it_request_id == rpc_client_instance_st.awaiter_forks_to_response.end()) [[unlikely]] {
    kphp::log::warning("awaiter forks {} in rpc queue isn't associated with response", *wait_result);
    co_return std::nullopt;
  }

  const int64_t ready_response_id{it_request_id->second};
  rpc_client_instance_st.awaiter_forks_to_response.erase(it_request_id);
  co_return ready_response_id;
}
} // namespace kphp::rpc

inline int64_t f$rpc_queue_create() noexcept {
  return f$wait_queue_create();
}

inline int64_t f$rpc_queue_create(const array<int64_t>& request_ids) noexcept {
  int64_t future{f$wait_queue_create()};
  for (auto request_id : request_ids) {
    kphp::rpc::rpc_queue_push(future, request_id.get_value());
  }

  return future;
}

inline void f$rpc_queue_push(int64_t queue_id, int64_t request_id) noexcept {
  return kphp::rpc::rpc_queue_push(queue_id, request_id);
}

inline bool f$rpc_queue_empty(int64_t queue_id) noexcept {
  return f$wait_queue_empty(queue_id);
}

inline kphp::coro::task<Optional<int64_t>> f$rpc_queue_next(int64_t queue_id, double timeout = -1) noexcept {
  auto opt_result{co_await kphp::forks::id_managed(
      kphp::rpc::rpc_queue_next(queue_id, std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{timeout})))};
  co_return opt_result ? *opt_result : Optional<int64_t>{};
}
