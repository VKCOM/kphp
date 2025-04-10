// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/job-workers/job-stats.h"
#include "server/job-workers/job-workers-context.h"
#include "server/job-workers/shared-memory-manager.h"
#include "server/workers-control.h"

#include "runtime/job-workers/processing-jobs.h"
#include "runtime/net_events.h"
#include "runtime/resumable.h"

#include "runtime/job-workers/job-interface.h"

namespace {

int job_timeout_wakeup_id{-1};

void process_job_timeout(kphp_event_timer* timer) {
  ::process_job_timeout(timer->wakeup_extra);
}

} // namespace

int get_job_timeout_wakeup_id() {
  return job_timeout_wakeup_id;
}

bool f$is_kphp_job_workers_enabled() noexcept {
  return f$get_job_workers_number() > 0;
}

int64_t f$get_job_workers_number() noexcept {
  return vk::singleton<WorkersControl>::get().get_count(WorkerType::job_worker);
}

void global_init_job_workers_lib() noexcept {
  if (f$is_kphp_job_workers_enabled()) {
    vk::singleton<job_workers::SharedMemoryManager>::get().init();
    job_timeout_wakeup_id = register_wakeup_callback(&process_job_timeout);
  }
}

void clear_shared_job_messages() noexcept {
  if (f$is_kphp_job_workers_enabled()) {
    vk::singleton<job_workers::SharedMemoryManager>::get().forcibly_release_all_attached_messages();
  }
}

void process_job_answer(int job_id, job_workers::FinishedJob* job_result) noexcept {
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

class_instance<C$KphpJobWorkerResponseError> f$KphpJobWorkerResponseError$$__construct(class_instance<C$KphpJobWorkerResponseError> const& v$this) noexcept {
  return v$this;
}

string f$KphpJobWorkerResponseError$$getError(class_instance<C$KphpJobWorkerResponseError> const& v$this) noexcept {
  return v$this.get()->error;
}

int64_t f$KphpJobWorkerResponseError$$getErrorCode(class_instance<C$KphpJobWorkerResponseError> const& v$this) noexcept {
  return v$this.get()->error_code;
}

class_instance<C$KphpJobWorkerResponseError> create_error_on_other_memory(int32_t error_code, const char* error_msg,
                                                                          memory_resource::unsynchronized_pool_resource& resource) noexcept {
  dl::set_current_script_allocator(resource, false);
  class_instance<C$KphpJobWorkerResponseError> error;
  assert(resource.is_enough_memory_for(sizeof(C$KphpJobWorkerResponseError)));
  error.alloc();

  const size_t error_msg_len = std::min(std::strlen(error_msg), size_t{512});
  assert(resource.is_enough_memory_for(string::estimate_memory_usage(error_msg_len)));
  error.get()->error = string{error_msg, static_cast<string::size_type>(error_msg_len)};
  error.get()->error_code = error_code;

  dl::restore_default_script_allocator(false);
  return error;
}

void C$KphpJobWorkerResponseError::accept(CommonMemoryEstimateVisitor& visitor) noexcept {
  return generic_accept(visitor);
}
