// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/job-workers/job-workers-context.h"
#include "server/job-workers/shared-context.h"

#include "runtime/job-workers/processing-jobs.h"
#include "runtime/job-workers/shared-memory-manager.h"

#include "runtime/job-workers/job-interface.h"

bool f$is_job_workers_enabled() noexcept {
  return vk::singleton<job_workers::JobWorkersContext>::get().job_workers_num > 0;
}

void global_init_job_workers_lib() noexcept {
  if (vk::singleton<job_workers::JobWorkersContext>::get().job_workers_num == 0) {
    return;
  }
  job_workers::SharedContext::get();
  vk::singleton<job_workers::SharedMemoryManager>::get().init();
}

void free_job_workers_lib() noexcept {
  vk::singleton<job_workers::ProcessingJobs>::get().reset();
}

void process_job_worker_answer_event(int ready_job_id, job_workers::SharedMemorySlice *job_result_script_memory_ptr) noexcept {
  vk::singleton<job_workers::ProcessingJobs>::get().finish_job_processing(ready_job_id, job_result_script_memory_ptr);
}
