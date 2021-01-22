// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <signal.h>

#include "common/dl-utils-lite.h"
#include "server/php-engine-vars.h"
#include "server/php-engine.h"
#include "server/php-master-restart.h"
#include "server/task-workers/task-worker-server.h"
#include "server/task-workers/task-workers-context.h"

int TaskWorkerServer::on_get_task(int fd, void *data __attribute__((unused)), event_t *ev) {
  static constexpr int QUERY_BUF_LEN = 100;
  static int buf[QUERY_BUF_LEN];

  vkprintf(3, "TaskWorkerClient::on_get_task: fd=%d\n", fd);

  const auto &task_worker_server = vk::singleton<TaskWorkerServer>::get();
  assert(fd == task_worker_server.read_task_fd);

  if (ev->ready & EVT_SPEC) {
    fprintf(stderr, "task worker special event: fd = %d, flags = %d\n", fd, ev->ready);
    // TODO:
    return 0;
  }
  assert(ev->ready & EVT_READ);

  ssize_t read_bytes = read(fd, buf, sizeof(int) * 3);
  assert(read_bytes == sizeof(int) * 3);

  int task_id = buf[0];
  int task_result_fd_idx = buf[1];
  int x = buf[2];
  
  tvkprintf(task_workers_logging, 2, "pid = %d, got task: task_id = %d, write_task_result_fd = %d\n", getpid(), task_id, task_result_fd_idx);

  int task_result = x * x;

  int write_task_result_fd = vk::singleton<TaskWorkersContext>::get().result_pipes.at(task_result_fd_idx)[1];

  size_t query_size = 0;
  buf[query_size++] = task_id;
  buf[query_size++] = task_result;
  ssize_t written = write(write_task_result_fd, buf, query_size * sizeof(int)); // TODO: make it non blocking
  if (written != query_size * sizeof(int)) {
    kprintf("Fail to write task result: written bytes = %zd %s\n", written, written == -1 ? strerror(errno) : "");
    assert(false);
  }
  return 0;
}

void TaskWorkerServer::init_task_worker_server() {
  prctl(PR_SET_PDEATHSIG, SIGKILL); // to prevent the process from becoming orphan

  if (getppid() != me->pid) {
    vkprintf(0, "parent is dead just after start\n");
    exit(124);
  }

  net_reset_after_fork();

  verbosity = initial_verbosity;
  ::pid = getpid();

  if (master_sfd >= 0) {
    close(master_sfd);
    clear_event(master_sfd); // TODO: maybe it's redundant
    master_sfd = -1;
  }

  if (http_sfd >= 0) {
    close(http_sfd);
    clear_event(http_sfd);
    http_sfd = -1;
  }

  const auto &task_workers_ctx = vk::singleton<TaskWorkersContext>::get();

  read_task_fd = task_workers_ctx.task_pipe[0];
  close(task_workers_ctx.task_pipe[1]); // this endpoint is for HTTP worker to write task
  clear_event(task_workers_ctx.task_pipe[1]);
  for (auto &result_pipe : task_workers_ctx.result_pipes) {
    close(result_pipe[0]); // this endpoint is for HTTP worker to read task result
    clear_event(result_pipe[0]);
  }
}

void TaskWorkerServer::event_loop() {
  init_task_worker_server();

  epoll_sethandler(read_task_fd, 0, on_get_task, nullptr);
  epoll_insert(read_task_fd, EVT_READ | EVT_SPEC);
  tvkprintf(task_workers_logging, 1, "insert read_task_fd = %d to epoll\n", read_task_fd);

  dl_allow_all_signals();

  while (true) {
    epoll_work(57);
  }

  epoll_close(read_task_fd);
  assert (close(read_task_fd) >= 0);
}
