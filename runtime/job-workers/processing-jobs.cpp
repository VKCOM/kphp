// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/job-workers/job-message.h"
#include "runtime/job-workers/shared-memory-manager.h"
#include "runtime/instance-copy-processor.h"

#include "runtime/job-workers/processing-jobs.h"

namespace job_workers {

void ProcessingJobs::finish_job_processing(job_workers::JobSharedMessage *job_result) noexcept {
  php_assert(job_result);
  auto reply = job_result->instance.cast_to<C$KphpJobWorkerResponse>();
  php_assert(!reply.is_null());

  const int job_slot_id = job_result->job_id;
  // TODO Check if reply is null => OOM
  reply = copy_instance_into_script_memory(reply);
  vk::singleton<SharedMemoryManager>::get().release_shared_message(job_result);
  auto &job_ready_result = processing_[job_slot_id];
  job_ready_result.reply = std::move(reply);
  job_ready_result.ready = true;
}

bool ProcessingJobs::is_ready(int job_slot_id) const noexcept {
  const ProcessingJobAwait *job_result = processing_.find_value(job_slot_id);
  return job_result && job_result->ready;
}

class_instance<C$KphpJobWorkerResponse> ProcessingJobs::withdraw(int job_slot_id) noexcept {
  class_instance<C$KphpJobWorkerResponse> result = std::move(processing_[job_slot_id].reply);
  processing_.unset(job_slot_id);
  return result;
}

} // namespace job_workers
