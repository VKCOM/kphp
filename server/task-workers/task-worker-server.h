// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <unistd.h>

#include "common/timer.h"
#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"
#include "net/net-reactor.h"
#include "server/task-workers/pipe-buffer.h"

/**
 * Task worker process runs task worker server
 */
class TaskWorkerServer : vk::not_copyable {
public:
  friend class vk::singleton<TaskWorkerServer>;
  static constexpr size_t TASK_BYTE_SIZE = 4 * sizeof(int); // task_id; write_task_result_fd; x; 0

  vk::SteadyTimer<std::chrono::milliseconds> last_stats;

  void init_task_worker_server();
  bool execute_task(int task_id, int task_result_fd_idx, int x);
  int read_execute_loop();

  void try_complete_delayed_tasks();

  static int read_tasks(int fd, void *data __attribute__((unused)), event_t *ev);

private:
  PipeBuffer buffer;
  bool has_delayed_tasks{false};
  int read_task_fd{-1};

  TaskWorkerServer() = default;
};
