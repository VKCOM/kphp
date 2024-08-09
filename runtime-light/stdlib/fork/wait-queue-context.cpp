// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/fork/wait-queue-context.h"

#include "runtime-light/component/component.h"
#include "runtime-light/stdlib/fork/fork-context.h"

WaitQueueContext &WaitQueueContext::get() noexcept {
  return ForkComponentContext::get().wait_queue_context;
}

int64_t WaitQueueContext::create_queue(const array<Optional<int64_t>> &fork_ids) noexcept {
  auto &memory_resource{get_component_context()->runtime_allocator.memory_resource};
  unordered_map<int64_t, task_t<fork_result>> forks(unordered_map<int64_t, task_t<fork_result>>::allocator_type{memory_resource});
  std::for_each(fork_ids.begin(), fork_ids.end(), [&forks](const auto &it) {
    Optional<int64_t> fork_id = it.get_value();
    if (fork_id.has_value()) {
      if (auto task = ForkComponentContext::get().pop_fork(fork_id.val()); task.has_value()) {
        forks[fork_id.val()] = std::move(task.val());
      }
    }
  });
  int64_t queue_id{++next_wait_queue_id};
  wait_queues.emplace(queue_id, WaitQueue(memory_resource, std::move(forks)));
  php_debug("WaitQueueContext: create queue %ld with %ld forks", queue_id, fork_ids.size().size);
  return queue_id;
}
