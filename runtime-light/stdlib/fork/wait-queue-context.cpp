// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/fork/wait-queue-context.h"

#include "runtime-light/component/component.h"
#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/stdlib/fork/fork-context.h"

WaitQueueContext &WaitQueueContext::get() noexcept {
  return ForkComponentContext::get().wait_queue_context;
}

int64_t WaitQueueContext::create_queue(const array<Optional<int64_t>> &fork_ids) noexcept {
  auto &memory_resource{get_component_context()->runtime_allocator.memory_resource};
  unordered_set<int64_t> forks_ids(unordered_set<int64_t>::allocator_type{memory_resource});
  std::for_each(fork_ids.begin(), fork_ids.end(), [&forks_ids](const auto &it) {
    Optional<int64_t> fork_id = it.get_value();
    if (fork_id.has_value()) {
      forks_ids.insert(fork_id.val());
    }
  });
  int64_t queue_id{++next_wait_queue_id};
  wait_queues.emplace(std::piecewise_construct, std::forward_as_tuple(queue_id), std::forward_as_tuple(memory_resource, std::move(forks_ids)));
  php_debug("WaitQueueContext: create queue %ld with %ld forks", queue_id, fork_ids.size().size);
  return queue_id;
}
