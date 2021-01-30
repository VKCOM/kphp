// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <sys/param.h>
#include <signal.h>

#include "common/dl-utils-lite.h"
#include "server/php-engine-vars.h"
#include "server/php-engine.h"
#include "server/php-master-restart.h"
#include "server/task-workers/task-worker-server.h"
#include "server/task-workers/task-workers-context.h"
#include "server/task-workers/shared-context.h"

int TaskWorkerServer::read_tasks(int fd, void *data __attribute__((unused)), event_t *ev) {
  static int read_buf[PIPE_BUF / sizeof(int)];

  vkprintf(3, "TaskWorkerClient::read_tasks: fd=%d\n", fd);
  tvkprintf(task_workers_logging, 3, "wakeup task worker\n");

  const auto &task_worker_server = vk::singleton<TaskWorkerServer>::get();
  assert(fd == task_worker_server.read_task_fd);

  if (ev->ready & EVT_SPEC) {
    fprintf(stderr, "task worker special event: fd = %d, flags = %d\n", fd, ev->ready);
    // TODO:
    return 0;
  }
  assert(ev->ready & EVT_READ);

  while (true) {
    ssize_t read_bytes = read(fd, read_buf, TASK_BYTE_SIZE);
    if (read_bytes == -1) {
      if (errno == EWOULDBLOCK) {
        // another task worker has already taken the task (all task workers are readers for this fd)
        // or there are no more tasks in pipe
        return 0;
      }
      kprintf("pid = %d, Couldn't read task: %s\n", getpid(), strerror(errno));
      return -1;
    }
    if (read_bytes != TASK_BYTE_SIZE) {
      kprintf("Couldn't read task. Got %zd bytes, but task bytes size = %zd. Probably some tasks are written not atomically\n", read_bytes, TASK_BYTE_SIZE);
      return -1;
    }
    int task_id = read_buf[0];
    int task_result_fd_idx = read_buf[1];
    int x = read_buf[2];
    int zero = read_buf[3];
    assert(zero == 0);

    SharedContext::get().task_queue_size--;

    bool success = task_worker_server.execute_task(task_id, task_result_fd_idx, x);
    assert(success);
  }
  return 0;
}

bool TaskWorkerServer::execute_task(int task_id, int task_result_fd_idx, int x) const {
  tvkprintf(task_workers_logging, 2, "executing task: task_id = %d, task_result_fd_idx = %d\n", task_id, task_result_fd_idx);

  static int write_buf[PIPE_BUF / sizeof(int)];

  int task_result = x * x;
  sleep(1);

  int write_task_result_fd = vk::singleton<TaskWorkersContext>::get().result_pipes.at(task_result_fd_idx)[1];
  size_t answer_size = 0;
  write_buf[answer_size++] = task_id;
  write_buf[answer_size++] = task_result;

  ssize_t written = write(write_task_result_fd, write_buf, answer_size * sizeof(int));
  if (written == -1) {
    if (errno == EWOULDBLOCK) {
      kprintf("Fail to write task result: pipe is full\n");
    } else {
      kprintf("Fail to write task result: %s", strerror(errno));
    }
    return false;
  }

  if (written != answer_size * sizeof(int)) {
    kprintf("Fail to write task result: written bytes = %zd\n", written);
    return false;
  }
  return true;
}


void TaskWorkerServer::init_task_worker_server() {
  const auto &task_workers_ctx = vk::singleton<TaskWorkersContext>::get();

  read_task_fd = task_workers_ctx.task_pipe[0];
  close(task_workers_ctx.task_pipe[1]); // this endpoint is for HTTP worker to write task
  clear_event(task_workers_ctx.task_pipe[1]);
  for (auto &result_pipe : task_workers_ctx.result_pipes) {
    close(result_pipe[0]); // this endpoint is for HTTP worker to read task result
    clear_event(result_pipe[0]);
  }

  epoll_sethandler(read_task_fd, 0, read_tasks, nullptr);
  epoll_insert(read_task_fd, EVT_READ | EVT_SPEC);
  tvkprintf(task_workers_logging, 1, "insert read_task_fd = %d to epoll\n", read_task_fd);
}
