// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-core/core-types/decl/optional.h"
#include "runtime-core/memory-resource/resource_allocator.h"
#include "runtime-core/memory-resource/unsynchronized_pool_resource.h"
#include "runtime-light/utils/concepts.h"
#include "runtime-light/stdlib/fork/wait-queue.h"

class WaitQueueContext {
  template<hashable Key, typename Value>
  using unordered_map = memory_resource::stl::unordered_map<Key, Value, memory_resource::unsynchronized_pool_resource>;

  template<hashable T>
  using unordered_set = memory_resource::stl::unordered_set<T, memory_resource::unsynchronized_pool_resource>;

  unordered_map<int64_t, wait_queue_t> wait_queues;
  static constexpr auto WAIT_QUEUE_INIT_ID = 0;
  int64_t next_wait_queue_id{WAIT_QUEUE_INIT_ID};

public:
  explicit WaitQueueContext(memory_resource::unsynchronized_pool_resource &memory_resource) noexcept
    : wait_queues(unordered_map<int64_t, wait_queue_t>::allocator_type{memory_resource}) {}

  static WaitQueueContext &get() noexcept;

  int64_t create_queue(const array<Optional<int64_t>> &fork_ids) noexcept;

  Optional<wait_queue_t *> get_queue(int64_t queue_id) noexcept {
    if (auto it = wait_queues.find(queue_id); it != wait_queues.end()) {
      return std::addressof(it->second);
    }
    return {};
  }
};
