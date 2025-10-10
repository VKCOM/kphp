// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <functional>
#include <optional>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/coroutine/await-set.h"

class WaitQueueInstanceState {
  static constexpr int64_t WAIT_QUEUE_ID_INIT = 0;
  int64_t next_wait_queue_id{WAIT_QUEUE_ID_INIT};
  kphp::stl::unordered_map<int64_t, kphp::coro::await_set<int64_t>, kphp::memory::script_allocator> queues{};

public:
  WaitQueueInstanceState() noexcept = default;

  static WaitQueueInstanceState& get() noexcept;

  [[nodiscard]] int64_t create_queue() noexcept {
    const int64_t wait_queue_id{next_wait_queue_id++};
    queues.emplace(wait_queue_id, kphp::coro::await_set<int64_t>{});
    return wait_queue_id;
  }

  std::optional<std::reference_wrapper<kphp::coro::await_set<int64_t>>> get_queue(int64_t queue_id) noexcept {
    if (auto it{queues.find(queue_id)}; it != queues.end()) {
      return it->second;
    }
    return std::nullopt;
  }
};
