// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <sys/param.h>
#include <cassert>
#include <cstdio>
#include <unistd.h>

#include "net/net-reactor.h"

#include "server/task-workers/task-worker-client.h"
#include "server/task-workers/task-workers-context.h"
#include "server/php-queries.h"

int TaskWorkerClient::read_task_results(int fd, void *data __attribute__((unused)), event_t *ev) {
  static int read_buf[PIPE_BUF / sizeof(int)];

  vkprintf(3, "TaskWorkerClient::read_task_results: fd=%d\n", fd);

  const auto &task_worker_client = vk::singleton<TaskWorkerClient>::get();
  assert(fd == task_worker_client.read_task_result_fd);
  if (ev->ready & EVT_SPEC) {
    fprintf(stderr, "task worker special event: fd = %d, flags = %d\n", fd, ev->ready);
    // TODO:
    return 0;
  }
  assert(ev->ready & EVT_READ);

  ssize_t read_bytes = read(fd, read_buf, PIPE_BUF);

  if (read_bytes == -1) {
    assert(errno != EWOULDBLOCK); // because fd must be ready for read, current HTTP worker is only one reader for this fd
    kprintf("Couldn't read task results: %s", strerror(errno));
    return -1;
  }
  if (read_bytes % TASK_RESULT_BYTE_SIZE != 0) {
    kprintf("Got inconsistent number of bytes on read task results: %zd. Probably some task results are written not atomically", read_bytes);
    return -1;
  }

  size_t tasks_results_number = read_bytes / TASK_RESULT_BYTE_SIZE;
  tvkprintf(task_workers_logging, 2, "got %zu task results, collecting ...\n", tasks_results_number);
  for (size_t i = 0, data_pos = 0; i < tasks_results_number; ++i) {
    int ready_task_id = read_buf[data_pos++];
    int task_result = read_buf[data_pos++];
    tvkprintf(task_workers_logging, 2, "collecting task result (%lu / %lu): ready_task_id = %d, task_result = %d\n", i + 1, tasks_results_number, ready_task_id, task_result);
    put_task_worker_answer_event(ready_task_id, task_result);
  }
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

bool TaskWorkerClient::send_task_x2(int task_id, int x) {
  static int write_buf[PIPE_BUF / sizeof(int)];
  tvkprintf(task_workers_logging, 2, "sending task: task_id = %d, write_task_fd = %d, task_result_fd_idx = %d\n", task_id, write_task_fd, task_result_fd_idx);

  size_t query_size = 0;
  write_buf[query_size++] = task_id;
  write_buf[query_size++] = task_result_fd_idx;
  write_buf[query_size++] = x;

  ssize_t written = write(write_task_fd, write_buf, query_size * sizeof(int));

  if (written == -1) {
    if (errno == EWOULDBLOCK) {
      kprintf("Fail to write task: pipe is full\n");
    } else {
      kprintf("Fail to write task: %s\n", strerror(errno));
    }
    return false;
  }

  if (written != query_size * sizeof(int)) {
    kprintf("Fail to write task: task_id = %d, written bytes = %zd\n", task_id, written);
    return false;
  }
  return true;
}
