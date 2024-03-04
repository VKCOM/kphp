// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "common/algorithms/find.h"
#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"
#include "server/job-workers/pipe-io.h"

typedef struct event_descr event_t;

namespace job_workers {

struct JobSharedMessage;

/**
 * HTTP worker process is usual job worker client
 */
class JobWorkerClient : vk::not_copyable {
public:
  friend class vk::singleton<JobWorkerClient>;

  int job_result_fd_idx{-1};
  int read_job_result_fd{-1};
  int write_job_fd{-1};
  PipeJobWriter job_writer;
  PipeJobReader job_reader;

  void init(int job_result_slot);

  bool is_inited() const {
    return vk::none_of_equal(-1, job_result_fd_idx, read_job_result_fd, write_job_fd);
  }

  bool send_job(JobSharedMessage *job_request);

private:
  JobWorkerClient() = default;

  static int read_job_results(int fd, void *data __attribute__((unused)), event_t *ev);
};

} // namespace job_workers
