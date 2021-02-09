// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/task-workers/shared-memory-manager.h"
#include "server/task-workers/pending-tasks.h"

namespace task_workers {

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

void PendingTasks::mark_task_ready(int task_id, void *task_result_script_memory_ptr) {
  const int64_t *task_result_memory = reinterpret_cast<int64_t *>(task_result_script_memory_ptr);
  int64_t arr_n = *task_result_memory++;
  array<int64_t> res;
  res.reserve(arr_n, 0, true);
  res.memcpy_vector(arr_n, task_result_memory); // TODO: create inplace instead of copying

  dl::deallocate(task_result_script_memory_ptr, vk::singleton<SharedMemoryManager>::get().get_slice_payload_size());

  tasks_.set_value(task_id, std::move(res));
}

array<int64_t> PendingTasks::withdraw_task(int task_id) {
  auto task_result = tasks_.get_value(task_id);
  tasks_.unset(task_id);
  return task_result.val();
}

void PendingTasks::reset() {
  tasks_ = {};
}

} // namespace task_workers
