// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/net_events.h"
#include "runtime/job-workers/job-message.h"
#include "runtime/job-workers/shared-memory-manager.h"
#include "runtime/instance-copy-processor.h"

#include "runtime/job-workers/processing-jobs.h"

namespace job_workers {

void ProcessingJobs::start_job_processing(int job_id, JobRequestInfo &&job_request_info) noexcept {
  processing_[job_id] = std::move(job_request_info);
}

int64_t ProcessingJobs::finish_job_on_answer(int job_id, job_workers::JobSharedMessage *job_result) noexcept {
  php_assert(job_result);
  php_assert(job_id == job_result->job_id);

  return finish_job_impl(job_id, job_result);
}

int64_t ProcessingJobs::finish_job_on_timeout(int job_id) noexcept {
  return finish_job_impl(job_id, nullptr);
}

int64_t ProcessingJobs::finish_job_impl(int job_id, job_workers::JobSharedMessage *job_result) noexcept {
  auto release_job_result = vk::finally([=]() {
    if (job_result) {
      vk::singleton<SharedMemoryManager>::get().release_shared_message(job_result);
    }
  });

  if (!processing_.has_key(job_id)) {
    // possible in case of answer after timeout
    return 0;
  }

  auto &ready_job = processing_[job_id];

  if (job_result) {
    auto reply = job_result->instance.cast_to<C$KphpJobWorkerResponse>();
    php_assert(!reply.is_null());

    // TODO Check if reply is null => OOM
    reply = copy_instance_into_script_memory(reply);
    ready_job.response = std::move(reply);
  } else {
    class_instance<C$KphpJobWorkerResponseError> error;
    error.alloc();  // TODO: handle OOM
    error.get()->error = string{"Job client timeout"};
    error.get()->error_code = -102;

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
