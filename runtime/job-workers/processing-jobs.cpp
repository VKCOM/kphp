// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/net_events.h"
#include "runtime/job-workers/job-message.h"
#include "runtime/job-workers/shared-memory-manager.h"
#include "runtime/instance-copy-processor.h"

#include "runtime/job-workers/processing-jobs.h"

namespace job_workers {

struct FinishedJob {
  class_instance<C$KphpJobWorkerResponse> response;
};

FinishedJob *copy_finished_job_to_script_memory(JobSharedMessage *job_message) noexcept {
  auto response = job_message->instance.cast_to<C$KphpJobWorkerResponse>();
  php_assert(!response.is_null());
  response = copy_instance_into_script_memory(response);
  if (!response.is_null()) {
    if (void *mem = dl::allocate(sizeof(FinishedJob))) {
      return new(mem) FinishedJob{std::move(response)};
    }
  }
  return nullptr;
}

void ProcessingJobs::start_job_processing(int job_id, JobRequestInfo &&job_request_info) noexcept {
  processing_[job_id] = std::move(job_request_info);
}

int64_t ProcessingJobs::finish_job_on_answer(int job_id, job_workers::FinishedJob *job_result) noexcept {
  return finish_job_impl(job_id, job_result, false);
}

int64_t ProcessingJobs::finish_job_on_timeout(int job_id) noexcept {
  return finish_job_impl(job_id, nullptr, true);
}

int64_t ProcessingJobs::finish_job_impl(int job_id, job_workers::FinishedJob *job_result, bool timeout) noexcept {
  if (!processing_.has_key(job_id)) {
    // possible in case of answer after timeout
    return 0;
  }

  auto &ready_job = processing_[job_id];

  if (job_result) {
    ready_job.response = std::move(job_result->response);
    job_result->~FinishedJob();
    dl::deallocate(job_result, sizeof(job_workers::FinishedJob));
  } else {
    class_instance<C$KphpJobWorkerResponseError> error;
    error.alloc();
    if (timeout) {
      error.get()->error = string{"Job client timeout"};
      error.get()->error_code = -102;
    } else {
      error.get()->error = string{"Not enough memory for accepting job response"};
      error.get()->error_code = -103;
    }
    ready_job.response = std::move(error);
  }

  if (ready_job.timer) {
    remove_event_timer(ready_job.timer);
  }

  return ready_job.resumable_id;
}
class_instance<C$KphpJobWorkerResponse> ProcessingJobs::withdraw(int job_id) noexcept {
  JobRequestInfo &ready_job = processing_[job_id];
  php_assert(ready_job.resumable_id != 0);
  class_instance<C$KphpJobWorkerResponse> result = std::move(ready_job.response);
  processing_.unset(job_id);
  return result;
}

} // namespace job_workers
