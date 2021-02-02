// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/task-workers/task-worker-server.h"
#include "server/php-engine-vars.h"
#include "server/php-engine.h"
#include "server/php-master-restart.h"
#include "server/task-workers/shared-context.h"
#include "server/task-workers/task-workers-context.h"

namespace task_workers {

int TaskWorkerServer::read_tasks(int fd, void *data __attribute__((unused)), event_t *ev) {
  vkprintf(3, "TaskWorkerClient::read_tasks: fd=%d\n", fd);
  tvkprintf(task_workers, 3, "wakeup task worker\n");

  auto &task_worker_server = vk::singleton<TaskWorkerServer>::get();
  assert(fd == task_worker_server.read_task_fd);

  if (ev->ready & EVT_SPEC) {
    fprintf(stderr, "task worker special event: fd = %d, flags = %d\n", fd, ev->ready);
    // TODO:
    return 0;
  }
  assert(ev->ready & EVT_READ);

  int status_code = task_worker_server.read_execute_loop();

  return status_code;
}

int TaskWorkerServer::read_execute_loop() {
  while (true) {
    if (last_stats.is_started() && last_stats.time() > std::chrono::duration<int>{TaskWorkersContext::MAX_HANGING_TIME_SEC / 2}) {
      tvkprintf(task_workers, 1, "Too many tasks. Task worker will complete them later. Goes back to event loop now\n");
      has_delayed_tasks = true;
      return 0;
    }
    ssize_t read_bytes = read(read_task_fd, buffer.read_buf, TASK_BYTE_SIZE);
    if (read_bytes == -1) {
      if (errno == EWOULDBLOCK) {
        // another task worker has already taken the task (all task workers are readers for this fd)
        // or there are no more tasks in pipe
        has_delayed_tasks = false;
        return 0;
      }
      kprintf("pid = %d, Couldn't read task: %s\n", getpid(), strerror(errno));
      return -1;
    }
    if (read_bytes != TASK_BYTE_SIZE) {
      kprintf("Couldn't read task. Got %zd bytes, but task bytes size = %zd. Probably some tasks are written not atomically\n", read_bytes, TASK_BYTE_SIZE);
      return -1;
    }
    int task_id = buffer.read_buf[0];
    int task_result_fd_idx = buffer.read_buf[1];
    int x = buffer.read_buf[2];
    int zero = buffer.read_buf[3];
    assert(zero == 0);

    SharedContext::get().task_queue_size--;

    bool success = execute_task(task_id, task_result_fd_idx, x);
    assert(success);
  }
}

bool TaskWorkerServer::execute_task(int task_id, int task_result_fd_idx, int x) {
  tvkprintf(task_workers, 2, "executing task: task_id = %d, task_result_fd_idx = %d\n", task_id, task_result_fd_idx);

  int task_result = x * x;
  sleep(1);

  int write_task_result_fd = vk::singleton<TaskWorkersContext>::get().result_pipes.at(task_result_fd_idx)[1];
  size_t answer_size = 0;
  buffer.write_buf[answer_size++] = task_id;
  buffer.write_buf[answer_size++] = task_result;

  ssize_t written = write(write_task_result_fd, buffer.write_buf, answer_size * sizeof(int));
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
  tvkprintf(task_workers, 1, "insert read_task_fd = %d to epoll\n", read_task_fd);
}

void TaskWorkerServer::try_complete_delayed_tasks() {
  if (has_delayed_tasks) {
    tvkprintf(task_workers, 1, "There are delayed tasks. Let's complete them\n");
    read_execute_loop();
  }
}

} // namespace task_workers
