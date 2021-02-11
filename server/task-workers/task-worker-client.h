// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"
#include "server/task-workers/pipe-io.h"

typedef struct event_descr event_t;

namespace task_workers {

/**
 * HTTP worker process is usual task worker client
 */
class TaskWorkerClient : vk::not_copyable {
public:
  friend class vk::singleton<TaskWorkerClient>;
  static constexpr int TASK_RESULT_BYTE_SIZE = 2 * sizeof(int64_t); // task_id; task_result_memory_ptr

  int task_result_fd_idx{-1};
  int read_task_result_fd{-1};
  int write_task_fd{-1};
  PipeTaskWriter task_writer;
  PipeTaskReader task_reader;

  void init_task_worker_client(int task_result_slot);

  int send_task(void *task_memory_ptr);

private:
  TaskWorkerClient() = default;

  static int read_task_results(int fd, void *data __attribute__((unused)), event_t *ev);
};

} // namespace task_workers
