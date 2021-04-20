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

  vk::SteadyTimer<std::chrono::milliseconds> last_stats;

  void init();

  int job_parse_execute(connection *c);

  void try_complete_delayed_jobs();

  void reset_running_job() noexcept;

  void try_store_job_response_error(const char *error_msg, int error_code);

  bool reply_is_expected() const noexcept {
    return running_job && !reply_was_sent;
  }

private:
  const char *send_job_reply(JobSharedMessage *response) noexcept;

  JobSharedMessage *running_job{nullptr};
  PipeJobWriter job_writer;
  PipeJobReader job_reader;
  bool has_delayed_jobs{false};
  int read_job_fd{-1};
  connection *read_job_connection{nullptr};
  bool reply_was_sent{false};

  JobWorkerServer() = default;
};

} // namespace job_workers
