// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <unistd.h>

#include "common/smart_ptrs/singleton.h"
#include "net/net-reactor.h"

/**
 * Task worker process runs task worker server
 */
class TaskWorkerServer {
public:
  friend class vk::singleton<TaskWorkerServer>;

  void init_task_worker_server();
  void event_loop();

  static int on_get_task(int fd, void *data __attribute__((unused)), event_t *ev);
private:
  int read_task_fd{-1};

  TaskWorkerServer() = default;
};
