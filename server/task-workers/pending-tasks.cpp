// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/task-workers/shared-memory-manager.h"
#include "server/task-workers/pending-tasks.h"

namespace task_workers {


int PendingTasks::generate_task_id() {
  return next_task_id_++;
}

void PendingTasks::put_task(int task_id) {
  tasks_.set_value(task_id, {});
}

bool PendingTasks::is_task_ready(int task_id) const {
  if (auto *task_result = tasks_.find_value(task_id)) {
    return task_result->has_value();
  } else {
    return false;
  }
}

bool PendingTasks::task_exists(int task_id) const {
  return tasks_.has_key(task_id);
}

void PendingTasks::mark_task_ready(int task_id, intptr_t task_result_memory_ptr) {
  void * const task_result_memory_slice = reinterpret_cast<void *>(task_result_memory_ptr);

  const int64_t *task_result_memory = reinterpret_cast<int64_t *>(task_result_memory_slice);
  int64_t arr_n = *task_result_memory++;
  array<int64_t> res;
  res.reserve(arr_n, 0, true);
  res.memcpy_vector(arr_n, task_result_memory);

  tasks_.set_value(task_id, std::move(res));
  vk::singleton<SharedMemoryManager>::get().deallocate_slice(task_result_memory_slice);
}

array<int64_t> PendingTasks::withdraw_task(int task_id) {
  auto task_result = tasks_.get_value(task_id);
  tasks_.unset(task_id);
  return task_result.val();
}

void PendingTasks::reset() {
  tasks_ = {};
  next_task_id_ = 0;
}

} // namespace task_workers
