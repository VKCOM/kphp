// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/task-workers/pending-tasks.h"

void PendingTasks::put_task(int task_id) {
  tasks_.set_value(task_id, -1);
}

bool PendingTasks::is_task_ready(int task_id) const {
  if (auto *task_result = tasks_.find_value(task_id)) {
    return *task_result >= 0;
  } else {
    return false;
  }
}

bool PendingTasks::task_exists(int task_id) const {
  return tasks_.has_key(task_id);
}

void PendingTasks::mark_task_ready(int task_id, int x2) {
  tasks_.set_value(task_id, x2);
}

int PendingTasks::withdraw_task(int task_id) {
  auto task_result = tasks_.get_value(task_id);
  tasks_.unset(task_id);
  return task_result;
}

void PendingTasks::reset() {
  tasks_ = {};
}
