// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"

#include "runtime-common/core/runtime-core.h"
#include "runtime/net_events.h"

#include "runtime/job-workers/job-interface.h"

namespace job_workers {

struct JobSharedMessage;

struct FinishedJob {
  class_instance<C$KphpJobWorkerResponse> response;
};

FinishedJob *copy_finished_job_to_script_memory(JobSharedMessage *job_message) noexcept;

struct JobRequestInfo {
  int64_t resumable_id{0};
  kphp_event_timer *timer{nullptr};
  class_instance<C$KphpJobWorkerResponse> response;

  JobRequestInfo() = default;

  JobRequestInfo(int64_t resumable_id, kphp_event_timer *timer)
    : resumable_id(resumable_id)
    , timer(timer) {}
};

class ProcessingJobs : vk::not_copyable {
public:
  void start_job_processing(int job_id, JobRequestInfo &&job_request_info) noexcept;

  int64_t finish_job_on_answer(int job_id, job_workers::FinishedJob *job_result) noexcept;
  int64_t finish_job_on_timeout(int job_id) noexcept;

  class_instance<C$KphpJobWorkerResponse> withdraw(int job_id) noexcept;

  void reset() noexcept {
    hard_reset_var(processing_);
  }

private:
  friend class vk::singleton<ProcessingJobs>;

  array<JobRequestInfo> processing_;

  ProcessingJobs() = default;

  int64_t finish_job_impl(int job_id, job_workers::FinishedJob *job_result, bool timeout) noexcept;
};

} // namespace job_workers
