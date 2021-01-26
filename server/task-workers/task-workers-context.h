// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/smart_ptrs/singleton.h"
#include "server/php-master.h"
#include <cstddef>
#include <queue>
#include <unordered_set>

#include "server/task-workers/task-worker-server.h"

DECLARE_VERBOSITY(task_workers_logging);

/**
 * Master process manages this context
 */
class TaskWorkersContext {
public:
  friend class vk::singleton<TaskWorkersContext>;
  using Pipe = std::array<int, 2>;

  size_t running_task_workers{0};
  size_t dying_task_workers{0};
  size_t task_workers_num{0};

  std::unordered_set<pid_t> task_workers;

  Pipe task_pipe;
  std::vector<Pipe> result_pipes;

  void master_init_pipes(int task_result_slots_num);
  void master_run_task_workers();

  int get_vacant_task_result_fd_idx();
  void put_back_task_result_fd_idx(int vacant_fd_idx);

private:
  size_t task_result_slots_num_{0};
  std::queue<int> task_result_fd_idx_vacant_pool_;
  bool pipes_inited_{false};

  void run_task_worker();
};
