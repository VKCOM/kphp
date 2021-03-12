// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/job-workers/job-workers-context.h"
#include "server/job-workers/job-stats.h"

#include "runtime/job-workers/processing-jobs.h"
#include "runtime/job-workers/shared-memory-manager.h"

#include "runtime/job-workers/job-interface.h"

bool f$is_kphp_job_workers_enabled() noexcept {
  return vk::singleton<job_workers::JobWorkersContext>::get().job_workers_num > 0;
}

void global_init_job_workers_lib() noexcept {
  if (f$is_kphp_job_workers_enabled()) {
    job_workers::JobStats::get();
    vk::singleton<job_workers::SharedMemoryManager>::get().init();
  }
}

void process_job_worker_answer_event(int ready_job_id, job_workers::SharedMemorySlice *job_result_memory_slice_ptr) noexcept {
  vk::singleton<job_workers::ProcessingJobs>::get().finish_job_processing(ready_job_id, job_result_memory_slice_ptr);
}
