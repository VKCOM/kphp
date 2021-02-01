// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"
#include "server/task-workers/pipe-buffer.h"

typedef struct event_descr event_t;

/**
 * HTTP worker process is usual task worker client
 */
class TaskWorkerClient : vk::not_copyable {
public:
  friend class vk::singleton<TaskWorkerClient>;
  static constexpr int TASK_RESULT_BYTE_SIZE = 2 * sizeof(int); // task_id; x^2

  int task_result_fd_idx{-1};
  int read_task_result_fd{-1};
  int write_task_fd{-1};
  PipeBuffer buffer;

  void init_task_worker_client(int task_result_slot);

  bool send_task_x2(int task_id, int x);

  static int read_task_results(int fd, void *data __attribute__((unused)), event_t *ev);
private:
  TaskWorkerClient() = default;
};
