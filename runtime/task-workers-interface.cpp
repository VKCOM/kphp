// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/task-workers-interface.h"
#include "runtime/net_events.h"
#include "server/task-workers/pending-tasks.h"
#include "server/task-workers/task-worker-client.h"
#include "server/task-workers/task-workers-context.h"
#include "server/task-workers/shared-context.h"
#include "server/task-workers/shared-memory-manager.h"

using namespace task_workers;

void init_task_workers_lib() {
  vk::singleton<PendingTasks>::get().reset();
}

void free_task_workers_lib() {
  vk::singleton<PendingTasks>::get().reset();
}

void global_init_task_workers_lib() {
  if (vk::singleton<TaskWorkersContext>::get().task_workers_num == 0) {
    return;
  }
  SharedContext::get();
  vk::singleton<SharedMemoryManager>::get().init();
}

void process_task_worker_answer_event(int ready_task_id, void *task_result_script_memory_ptr) {
  vk::singleton<PendingTasks>::get().mark_task_ready(ready_task_id, task_result_script_memory_ptr);
}

int64_t f$async_x2(const array<int64_t> &arr) {
  if (!f$is_task_workers_enabled()) {
    php_warning("Can't send task: task workers disabled");
    return -1;
  }
  auto &memory_manager = vk::singleton<SharedMemoryManager>::get();
  void * const memory_slice = memory_manager.allocate_slice();
  if (memory_slice == nullptr) {
    php_warning("Can't allocate slice for task: not enough shared memory");
    SharedContext::get().total_errors_shared_memory_limit++;
    return -1;
  }

  auto *task_memory = reinterpret_cast<int64_t *>(memory_slice);
  *task_memory++ = arr.count();
  memcpy(task_memory, arr.get_const_vector_pointer(), arr.count() * sizeof(int64_t));

  int task_id = vk::singleton<TaskWorkerClient>::get().send_task(memory_slice);
  if (task_id <= 0) {
    memory_manager.deallocate_slice(memory_slice);
    php_warning("Can't send task. Probably tasks queue is full");
    return -1;
  }

  vk::singleton<PendingTasks>::get().put_task(task_id);
  return task_id;
}

array<int64_t> f$await_x2(int64_t task_id) {
  if (!f$is_task_workers_enabled()) {
    php_warning("Can't wait task: task workers disabled");
    return {};
  }
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

bool f$is_task_workers_enabled() {
  return vk::singleton<TaskWorkersContext>::get().task_workers_num > 0;
}
