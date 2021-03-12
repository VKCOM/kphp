// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/job-workers/shared-memory-manager.h"
#include "runtime/instance-copy-processor.h"

#include "runtime/job-workers/processing-jobs.h"

namespace job_workers {

void ProcessingJobs::finish_job_processing(int job_slot_id, SharedMemorySlice *reply_slice) noexcept {
  php_assert(reply_slice);
  auto reply = reply_slice->instance.cast_to<C$KphpJobWorkerResponse>();
  php_assert(!reply.is_null());

  // TODO Check if reply is null => OOM
  reply = copy_instance_into_script_memory(reply);
  vk::singleton<SharedMemoryManager>::get().release_slice(reply_slice);
  auto &job_result = processing_[job_slot_id];
  job_result.reply = std::move(reply);
  job_result.ready = true;
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
