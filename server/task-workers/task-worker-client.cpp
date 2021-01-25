// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cassert>
#include <cstdio>
#include <unistd.h>

#include "net/net-reactor.h"

#include "server/php-engine.h"
#include "server/task-workers/task-worker-client.h"
#include "server/task-workers/task-workers-context.h"
#include "server/task-workers/pending-tasks.h"


int TaskWorkerClient::on_get_task_result(int fd, void *data __attribute__((unused)), event_t *ev) {
  static constexpr int MAX_QUERY_INT_LEN = 100;
  static int buf[MAX_QUERY_INT_LEN];

  vkprintf(3, "TaskWorkerClient::on_get_task_result: fd=%d\n", fd);

  const auto &task_worker_client = vk::singleton<TaskWorkerClient>::get();
  assert(fd == task_worker_client.read_task_result_fd);
  if (ev->ready & EVT_SPEC) {
    fprintf(stderr, "task worker special event: fd = %d, flags = %d\n", fd, ev->ready);
    // TODO:
    return 0;
  }
  assert(ev->ready & EVT_READ);

  ssize_t read_bytes = read(fd, buf, sizeof(int) * 2);
  assert(read_bytes == sizeof(int) * 2);
  int ready_task_id = buf[0];
  int task_result = buf[1];

  tvkprintf(task_workers_logging, 2, "got task result: ready_task_id = %d\n", ready_task_id);

  vk::singleton<PendingTasks>::get().mark_task_ready(ready_task_id, task_result);
  return 0;
}

void TaskWorkerClient::init_task_worker_client(int task_result_slot) {
  auto &task_workers_ctx = vk::singleton<TaskWorkersContext>::get();

  write_task_fd = task_workers_ctx.task_pipe[1];
  close(task_workers_ctx.task_pipe[0]); // this endpoint is for task worker to read task
  clear_event(task_workers_ctx.task_pipe[0]);

  for (int i = 0; i < task_workers_ctx.result_pipes.size(); ++i) {
    auto &result_pipe = task_workers_ctx.result_pipes[i];
    if (i == task_result_slot) {
      read_task_result_fd = result_pipe[0];
      task_result_fd_idx = task_result_slot;
    } else {
      close(result_pipe[0]); // this endpoint is for another HTTP worker to read task result
      clear_event(result_pipe[0]);
    }
    close(result_pipe[1]); // this endpoint is for task worker to write task result
    clear_event(result_pipe[1]);
  }
}

void TaskWorkerClient::send_task_x2(int task_id, int x) {
  tvkprintf(task_workers_logging, 2, "sending task: task_id = %d, write_task_fd = %d, task_result_fd_idx = %d\n", task_id, write_task_fd, task_result_fd_idx);

  size_t query_size = 0;
  buf[query_size++] = task_id;
  buf[query_size++] = task_result_fd_idx;
  buf[query_size++] = x;

  ssize_t written = write(write_task_fd, buf, query_size * sizeof(int));
  if (written != query_size * sizeof(int)) {
    kprintf("Fail to write task: task_id = %d, written bytes = %zd %s\n", task_id, written, written == -1 ? strerror(errno) : "");
    assert(false);
  }
}
