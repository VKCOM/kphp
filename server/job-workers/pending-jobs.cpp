// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/job-workers/pending-jobs.h"
#include "server/job-workers/shared-memory-manager.h"

namespace job_workers {

void PendingJobs::put_job(int job_id) {
  jobs_.set_value(job_id, {});
}

bool PendingJobs::is_job_ready(int job_id) const {
  if (auto *job_result = jobs_.find_value(job_id)) {
    return job_result->has_value();
  } else {
    return false;
  }
}

bool PendingJobs::job_exists(int job_id) const {
  return jobs_.has_key(job_id);
}

void PendingJobs::mark_job_ready(int job_id, void *job_result_script_memory_ptr) {
  const int64_t *job_result_memory = reinterpret_cast<int64_t *>(job_result_script_memory_ptr);
  int64_t arr_n = *job_result_memory++;
  array<int64_t> res;
  res.reserve(arr_n, 0, true);
  res.memcpy_vector(arr_n, job_result_memory); // TODO: create inplace instead of copying

  dl::deallocate(job_result_script_memory_ptr, vk::singleton<SharedMemoryManager>::get().get_slice_payload_size());

  jobs_.set_value(job_id, std::move(res));
}

array<int64_t> PendingJobs::withdraw_job(int job_id) {
  auto job_result = jobs_.get_value(job_id);
  jobs_.unset(job_id);
  return job_result.val();
}

void PendingJobs::reset() {
  jobs_ = {};
}

} // namespace job_workers
