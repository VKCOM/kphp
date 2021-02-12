// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <climits>

namespace task_workers {

struct Task {
  int task_id{};
  int task_result_fd_idx{};
  void *task_memory_ptr{};
};

static_assert(PIPE_BUF % sizeof(Task) == 0, "Size of task must be multiple of PIPE_BUF for read/write atomicity");

struct TaskResult {
  int padding{};
  int task_id{};
  void *task_result_memory_ptr{};
};

static_assert(PIPE_BUF % sizeof(TaskResult) == 0, "Size of task result must be multiple of PIPE_BUF for read/write atomicity");

} // namespace task_workers
