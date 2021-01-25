// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/task-workers/task-workers.h"
#include "server/task-workers/pending-tasks.h"


void init_task_workers_lib() {
  vk::singleton<PendingTasks>::get().reset();
}

void free_task_workers_lib() {
  vk::singleton<PendingTasks>::get().reset();
}

