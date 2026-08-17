// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <functional>
#include <optional>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/coroutine/await-set.h"

class RpcQueueInstanceState final : private vk::not_copyable {
  static constexpr int64_t RPC_QUEUE_ID_INIT = 0;
  int64_t m_rpc_wait_queue_id{RPC_QUEUE_ID_INIT};
  kphp::stl::unordered_map<int64_t, kphp::coro::await_set<int64_t>, kphp::memory::script_allocator> m_queues;

public:
  RpcQueueInstanceState() noexcept = default;

  static RpcQueueInstanceState& get() noexcept;

  [[nodiscard]] int64_t create_queue() noexcept {
    const int64_t wait_queue_id{m_rpc_wait_queue_id++};
    m_queues.emplace(wait_queue_id, kphp::coro::await_set<int64_t>{});
    return wait_queue_id;
  }

  std::optional<std::reference_wrapper<kphp::coro::await_set<int64_t>>> get_queue(int64_t queue_id) noexcept {
    if (auto it{m_queues.find(queue_id)}; it != m_queues.end()) {
      return it->second;
    }
    return std::nullopt;
  }
};
