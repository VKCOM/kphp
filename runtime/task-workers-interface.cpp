// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/task-workers-interface.h"
#include "runtime/net_events.h"
#include "server/task-workers/pending-tasks.h"
#include "server/task-workers/task-worker-client.h"
#include "server/task-workers/shared-memory-manager.h"

using namespace task_workers;

void process_task_worker_answer_event(int ready_task_id, intptr_t task_result_memory_ptr) {
  vk::singleton<PendingTasks>::get().mark_task_ready(ready_task_id, task_result_memory_ptr);
}

int64_t f$async_x2(const array<int64_t> &arr) {
  void * const memory_slice = vk::singleton<SharedMemoryManager>::get().allocate_slice();

  auto *task_memory = reinterpret_cast<int64_t *>(memory_slice);
  *task_memory++ = arr.count();
  memcpy(task_memory, arr.get_const_vector_pointer(), arr.count() * sizeof(int64_t));

  auto &pending_tasks = vk::singleton<PendingTasks>::get();
  int task_id = pending_tasks.generate_task_id();

  bool success = vk::singleton<TaskWorkerClient>::get().send_task(task_id, reinterpret_cast<intptr_t>(memory_slice));
  php_assert(success);

  pending_tasks.put_task(task_id);
  return task_id;
}

array<int64_t> f$await_x2(int64_t task_id) {
  auto &pending_tasks = vk::singleton<PendingTasks>::get();
  if (!pending_tasks.task_exists(task_id)) {
    php_warning("Task with id %" PRIi64 " doesn't exist", task_id);
    return {};
  }
  while (!pending_tasks.is_task_ready(task_id)) {
    wait_net(0);
  }
  return pending_tasks.withdraw_task(task_id);
}
