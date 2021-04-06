// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/job-workers/job-stats.h"
#include "server/job-workers/job-workers-context.h"

#include "runtime/job-workers/processing-jobs.h"
#include "runtime/job-workers/shared-memory-manager.h"
#include "runtime/resumable.h"
#include "runtime/net_events.h"

#include "runtime/job-workers/job-interface.h"

namespace {

int job_timeout_wakeup_id{-1};

void process_job_timeout(kphp_event_timer *timer) {
  ::process_job_timeout(timer->wakeup_extra);
}

} // namespace

int get_job_timeout_wakeup_id() {
  return job_timeout_wakeup_id;
}

bool f$is_kphp_job_workers_enabled() noexcept {
  return vk::singleton<job_workers::JobWorkersContext>::get().job_workers_num > 0;
}

void global_init_job_workers_lib() noexcept {
  // Always init the job stats, because the instance of stats can be used without job workers
  job_workers::JobStats::get();
  if (f$is_kphp_job_workers_enabled()) {
    vk::singleton<job_workers::SharedMemoryManager>::get().init();
    job_timeout_wakeup_id = register_wakeup_callback(&process_job_timeout);
  }
}

void process_job_answer(int job_id, job_workers::JobSharedMessage *job_result) noexcept {
  int64_t job_resumable_id = vk::singleton<job_workers::ProcessingJobs>::get().finish_job_on_answer(job_id, job_result);

  if (job_resumable_id == 0) {
    return;
  }

  resumable_run_ready(job_resumable_id);
}

void process_job_timeout(int job_id) noexcept {
  int64_t job_resumable_id = vk::singleton<job_workers::ProcessingJobs>::get().finish_job_on_timeout(job_id);

  if (job_resumable_id == 0) {
    return;
  }

  resumable_run_ready(job_resumable_id);
}

class_instance<C$KphpJobWorkerResponseError> f$KphpJobWorkerResponseError$$__construct(class_instance<C$KphpJobWorkerResponseError> const &v$this) noexcept {
  return v$this;
}

string f$KphpJobWorkerResponseError$$getError(class_instance<C$KphpJobWorkerResponseError> const &v$this) noexcept {
  return v$this.get()->error;
}

int64_t f$KphpJobWorkerResponseError$$getErrorCode(class_instance<C$KphpJobWorkerResponseError> const &v$this) noexcept {
  return v$this.get()->error_code;
}
