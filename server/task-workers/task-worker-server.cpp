// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

#include "common/kprintf.h"

#include "net/net-events.h"
#include "net/net-reactor.h"

#include "server/task-workers/task-worker-server.h"
#include "server/php-master-restart.h"
#include "server/task-workers/shared-context.h"
#include "server/task-workers/shared-memory-manager.h"
#include "server/task-workers/task-workers-context.h"

namespace task_workers {

int TaskWorkerServer::read_tasks(int fd, void *data __attribute__((unused)), event_t *ev) {
  vkprintf(3, "TaskWorkerClient::read_tasks: fd=%d\n", fd);
  tvkprintf(task_workers, 3, "wakeup task worker\n");

  auto &task_worker_server = vk::singleton<TaskWorkerServer>::get();
  assert(fd == task_worker_server.read_task_fd);

  if (ev->ready & EVT_SPEC) {
    kprintf("task worker server special event: fd = %d, flags = %d\n", fd, ev->ready);
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

    task_reader.reset();
    ssize_t read_bytes = task_reader.read_task_from_pipe(TASK_BYTE_SIZE);

    if (read_bytes == -2) {
      assert(errno == EWOULDBLOCK);
      // another task worker has already taken the task (all task workers are readers for this fd)
      // or there are no more tasks in pipe
      has_delayed_tasks = false;
      return 0;
    } else if (read_bytes < 0) {
      return -1;
    }

    int64_t qword = task_reader.next();
    int task_id = qword >> 32;
    int task_result_fd_idx = qword & 0xFFFFFFFF;
    auto *task_memory_ptr = reinterpret_cast<void *>(task_reader.next());

    SharedContext::get().task_queue_size--;

    bool success = execute_task(task_id, task_result_fd_idx, task_memory_ptr);

    if (!success) {
      kprintf("Error on executing task: <task_result_fd_idx, task_id> = <%d, %d>, task_memory_ptr = %p\n", task_result_fd_idx, task_id, task_memory_ptr);
    }
  }
}

bool TaskWorkerServer::execute_task(int task_id, int task_result_fd_idx, void * const task_memory_ptr) {
  tvkprintf(task_workers, 2, "executing task: <task_result_fd_idx, task_id> = <%d, %d>, task_memory_ptr = %p\n", task_result_fd_idx, task_id, task_memory_ptr);

  SharedMemoryManager &memory_manager = vk::singleton<SharedMemoryManager>::get();

  auto *task_memory = static_cast<int64_t *>(task_memory_ptr);
  int64_t arr_n = *task_memory++;
  std::vector<int64_t> res(arr_n);
  for (int i = 0; i < arr_n; ++i) {
    res[i] = task_memory[i] * task_memory[i];
  }
  memory_manager.deallocate_slice(task_memory_ptr);

  void * const task_result_memory_slice = memory_manager.allocate_slice(); // TODO: reuse task_memory_ptr ?
  if (task_result_memory_slice == nullptr) {
    kprintf("Can't allocate slice for result of task: not enough shared memory\n");
    return false;
  }

  auto *task_result_memory = reinterpret_cast<int64_t *>(task_result_memory_slice);
  *task_result_memory++ = arr_n;
  memcpy(task_result_memory, res.data(), res.size() * sizeof(int64_t));

  int write_task_result_fd = vk::singleton<TaskWorkersContext>::get().result_pipes.at(task_result_fd_idx)[1];

  task_writer.reset();
  task_writer.append(task_id);
  task_writer.append(reinterpret_cast<intptr_t>(task_result_memory_slice));
  bool success = task_writer.flush_to_pipe(write_task_result_fd, "writing result of task");
  if (!success) {
    memory_manager.deallocate_slice(task_result_memory_slice);
    return false;
  }

  SharedContext::get().total_tasks_done_count++;

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

  task_reader = PipeTaskReader{read_task_fd};
}

void TaskWorkerServer::try_complete_delayed_tasks() {
  if (has_delayed_tasks) {
    tvkprintf(task_workers, 1, "There are delayed tasks. Let's complete them\n");
    read_execute_loop();
  }
}

} // namespace task_workers
