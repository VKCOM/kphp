// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/smart_ptrs/singleton.h"

typedef struct event_descr event_t;

/**
 * HTTP worker process is usual task worker client
 */
class TaskWorkerClient {
public:
  friend class vk::singleton<TaskWorkerClient>;

  int task_result_fd_idx{-1};
  int read_task_result_fd{-1};
  int write_task_fd{-1};

  void init_task_worker_client(int task_result_slot);

  void send_task_x2(int task_id, int x);

  static int on_get_task_result(int fd, void *data __attribute__((unused)), event_t *ev);
private:
  static constexpr int QUERY_BUF_LEN = 100;
  int buf[QUERY_BUF_LEN]{};

  TaskWorkerClient() = default;
};
