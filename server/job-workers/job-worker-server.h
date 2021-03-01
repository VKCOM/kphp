// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <unistd.h>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"
#include "common/timer.h"
#include "server/job-workers/job.h"
#include "server/job-workers/pipe-io.h"

typedef struct event_descr event_t;

namespace job_workers {

/**
 * Job worker process runs job worker server
 */
class JobWorkerServer : vk::not_copyable {
public:
  friend class vk::singleton<JobWorkerServer>;

  vk::SteadyTimer<std::chrono::milliseconds> last_stats;

  void init();
  bool execute_job(const Job &job);
  int read_execute_loop();

  void try_complete_delayed_jobs();

private:
  PipeJobWriter job_writer;
  PipeJobReader job_reader;
  bool has_delayed_jobs{false};
  int read_job_fd{-1};

  JobWorkerServer() = default;

  static int read_jobs(int fd, void *data __attribute__((unused)), event_t *ev);
};

} // namespace job_workers
