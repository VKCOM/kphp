// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <unistd.h>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"
#include "common/timer.h"
#include "server/task-workers/pipe-io.h"

typedef struct event_descr event_t;

namespace task_workers {

/**
 * Task worker process runs task worker server
 */
class TaskWorkerServer : vk::not_copyable {
public:
  friend class vk::singleton<TaskWorkerServer>;
  static constexpr size_t TASK_BYTE_SIZE = 2 * sizeof(int64_t); // <task_id, write_task_result_fd>; task_memory_ptr

  vk::SteadyTimer<std::chrono::milliseconds> last_stats;

  void init_task_worker_server();
  bool execute_task(int task_id, int task_result_fd_idx, intptr_t task_memory_ptr);
  int read_execute_loop();

  void try_complete_delayed_tasks();

private:
  PipeTaskWriter task_writer;
  PipeTaskReader task_reader;
  bool has_delayed_tasks{false};
  int read_task_fd{-1};

  TaskWorkerServer() = default;

  static int read_tasks(int fd, void *data __attribute__((unused)), event_t *ev);
};

} // namespace task_workers
