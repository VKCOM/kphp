// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <unistd.h>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"
#include "common/timer.h"
#include "server/job-workers/pipe-io.h"

struct connection;

namespace job_workers {

struct JobSharedMessage;

/**
 * Job worker process runs job worker server
 */
class JobWorkerServer : vk::not_copyable {
public:
  friend class vk::singleton<JobWorkerServer>;

  struct JobRequestStat {
    double job_wait_time{0};
    size_t job_request_max_real_memory_used{0};
    size_t job_request_max_memory_used{0};
    size_t job_response_max_real_memory_used{0};
    size_t job_response_max_memory_used{0};
  };

  JobRequestStat job_stat;

  void init() noexcept;

  void rearm_read_job_fd() noexcept;

  int job_parse_execute(connection *c) noexcept;

  void reset_running_job() noexcept;

  void store_job_response_error(const char *error_msg, int error_code) noexcept;

  void flush_job_stat() noexcept;

  bool reply_is_expected() const noexcept;

private:
  const char *send_job_reply(JobSharedMessage *response) noexcept;

  JobSharedMessage *running_job{nullptr};
  PipeJobWriter job_writer;
  PipeJobReader job_reader;
  int read_job_fd{-1};
  connection *read_job_connection{nullptr};
  bool reply_was_sent{false};

  JobWorkerServer() = default;
};

} // namespace job_workers
