// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/task-workers-interface.h"
#include "runtime/net_events.h"
#include "runtime/math_functions.h"
#include "server/task-workers/pending-tasks.h"
#include "server/task-workers/task-worker-client.h"

using namespace task_workers;

void process_task_worker_answer_event(int ready_task_id, int task_result) {
  vk::singleton<PendingTasks>::get().mark_task_ready(ready_task_id, task_result);
}

static int64_t generate_id() {
  return f$rand();
}

int64_t f$async_x2(int64_t x) {
  int task_id = static_cast<int>(generate_id());
  vk::singleton<PendingTasks>::get().put_task(task_id);
  bool success = vk::singleton<TaskWorkerClient>::get().send_task_x2(task_id, x);
  php_assert(success);
  return task_id;
}

int64_t f$await_x2(int64_t task_id) {
  auto &pending_tasks = vk::singleton<PendingTasks>::get();
  if (!pending_tasks.task_exists(task_id)) {
    php_warning("Task with id %" PRIi64 "doesn't exist", task_id);
    return -1;
  }
  while (!pending_tasks.is_task_ready(task_id)) {
    wait_net(0);
  }
  int task_result = pending_tasks.withdraw_task(task_id);
  return task_result;
}
