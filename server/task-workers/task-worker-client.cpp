// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cassert>
#include <unistd.h>

#include "common/kprintf.h"

#include "net/net-events.h"
#include "net/net-reactor.h"

#include "server/php-queries.h"
#include "server/task-workers/shared-context.h"
#include "server/task-workers/task-worker-client.h"
#include "server/task-workers/task-workers-context.h"

namespace task_workers {

int TaskWorkerClient::read_task_results(int fd, void *data __attribute__((unused)), event_t *ev) {
  vkprintf(3, "TaskWorkerClient::read_task_results: fd=%d\n", fd);

  auto &task_worker_client = vk::singleton<TaskWorkerClient>::get();
  assert(fd == task_worker_client.read_task_result_fd);
  if (ev->ready & EVT_SPEC) {
    kprintf("task worker client special event: fd = %d, flags = %d\n", fd, ev->ready);
    // TODO:
    return 0;
  }
  if (!(ev->ready & EVT_READ)) {
    kprintf("Strange event in client: fd = %d, ev->ready = 0x%08x\n", fd, ev->ready);
    return 0;
  }

  PipeTaskReader::ReadStatus status{};

  do {
    TaskResult task_result;
    status = task_worker_client.task_reader.read_task_result(task_result);
    if (status == PipeTaskReader::READ_FAIL) {
      SharedContext::get().total_errors_pipe_client_read++;
      return -1;
    }
    if (status == PipeTaskReader::READ_OK) {
      tvkprintf(task_workers, 2, "got task result: ready_task_id = %d, task_result_memory_ptr = %p\n", task_result.task_id, task_result.task_result_memory_ptr);
      create_task_worker_answer_event(task_result.task_id, task_result.task_result_memory_ptr);
    }
  } while (status != PipeTaskReader::READ_BLOCK);

  return 0;
}

void TaskWorkerClient::init_task_worker_client(int task_result_slot) {
  auto &task_workers_ctx = vk::singleton<TaskWorkersContext>::get();

  assert(task_workers_ctx.pipes_inited);

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

  if (read_task_result_fd >= 0) {
    epoll_sethandler(read_task_result_fd, 0, TaskWorkerClient::read_task_results, nullptr);
    epoll_insert(read_task_result_fd, EVT_READ | EVT_SPEC);
    task_reader = PipeTaskReader{read_task_result_fd};
  }
}

int TaskWorkerClient::send_task(void * const task_memory_ptr) {
  static_assert(sizeof(task_memory_ptr) == 8, "Unexpected pointer size");

  slot_id_t task_id = create_slot();

  tvkprintf(task_workers, 2, "sending task: <task_result_fd_idx, task_id> = <%d, %d> , task_memory_ptr = %p, write_task_fd = %d\n", task_result_fd_idx,
            task_id, task_memory_ptr, write_task_fd);

  bool success = task_writer.write_task(Task{task_id, task_result_fd_idx, task_memory_ptr}, write_task_fd);
  if (!success) {
    SharedContext::get().total_errors_pipe_client_write++;
    return -1;
  }

  SharedContext::get().task_queue_size++;
  SharedContext::get().total_tasks_sent++;
  return task_id;
}

} // namespace task_workers
