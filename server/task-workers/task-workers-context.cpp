// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <unistd.h>
#include <fcntl.h>

#include "common/dl-utils-lite.h"
#include "server/task-workers/task-workers-context.h"
#include "server/task-workers/task-worker-server.h"

DEFINE_VERBOSITY(task_workers_logging);

void TaskWorkersContext::master_init_pipes(int task_result_slots_num) {
  if (pipes_inited_) {
    return;
  }

  int err = pipe2(task_pipe.data(), O_NONBLOCK);
  if (err) {
    kprintf("Unable to create task pipe: %s", strerror(errno));
    assert(false);
    return;
  }

  result_pipes.resize(task_result_slots_num);
  for (int i = 0; i < result_pipes.size(); ++i) {
    auto &result_pipe = result_pipes.at(i);
    err = pipe2(result_pipe.data(), O_NONBLOCK);
    if (err) {
      kprintf("Unable to create result pipe: %s", strerror(errno));
      assert(false);
      return;
    }
  }

  task_result_slots_num_ = task_result_slots_num;
  pipes_inited_ = true;
}

void TaskWorkersContext::master_run_task_workers() {
  for (int i = 0; i < task_workers_num - (running_task_workers + dying_task_workers); ++i) {
    run_task_worker();
  }
}

void TaskWorkersContext::run_task_worker() {
  dl_block_all_signals();

  pid_t pid = fork();
  if (pid == -1) {
    kprintf("Failed to start task worker: %s\n", strerror(errno));
    assert(false);
  }
  if (pid != 0) {
    // in master
    tvkprintf(task_workers_logging, 1, "launched new task worker with pid = %d\n", pid);
    task_workers.insert(pid);
    ++running_task_workers;
    dl_restore_signal_mask();
    return;
  }
  // in task worker

  vk::singleton<TaskWorkerServer>::get().event_loop();

  exit(0);
}
