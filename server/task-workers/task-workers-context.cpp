// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <fcntl.h>
#include <unistd.h>

#include "server/task-workers/task-workers-context.h"

DEFINE_VERBOSITY(task_workers);

namespace task_workers {

void TaskWorkersContext::master_init_pipes(int task_result_slots_num) {
  if (pipes_inited_) {
    return;
  }

  int err = pipe2(task_pipe.data(), O_NONBLOCK);
  if (err) {
    kprintf("Unable to create task pipe: %s", strerror(errno));
    assert(false);
    return;
  }

  result_pipes.resize(task_result_slots_num);
  for (int i = 0; i < result_pipes.size(); ++i) {
    auto &result_pipe = result_pipes.at(i);
    err = pipe2(result_pipe.data(), O_NONBLOCK);
    if (err) {
      kprintf("Unable to create result pipe: %s", strerror(errno));
      assert(false);
      return;
    }
  }

  pipes_inited_ = true;
}

} // namespace task_workers
