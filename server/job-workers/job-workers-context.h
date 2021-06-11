// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cstddef>
#include <queue>
#include <unordered_set>

#include "common/kprintf.h"
#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"
#include "server/job-workers/job-worker-server.h"

DECLARE_VERBOSITY(job_workers);

namespace job_workers {

/**
 * Master process manages this context
 */
class JobWorkersContext : vk::not_copyable {
public:
  friend class vk::singleton<JobWorkersContext>;
  using Pipe = std::array<int, 2>;

  Pipe job_pipe{};
  std::vector<Pipe> result_pipes;
  bool pipes_inited{false};

  void master_init_pipes(int job_result_slots_num);

private:
  JobWorkersContext() {
    job_pipe.fill(-1);
    for (auto &result_pipe : result_pipes) {
      result_pipe.fill(-1);
    }
  }
};

} // namespace job_workers
