// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/job-workers-interface.h"
#include "runtime/net_events.h"
#include "server/job-workers/job-worker-client.h"
#include "server/job-workers/job-workers-context.h"
#include "server/job-workers/pending-jobs.h"
#include "server/job-workers/shared-context.h"
#include "server/job-workers/shared-memory-manager.h"

using namespace job_workers;

void init_job_workers_lib() {
  vk::singleton<PendingJobs>::get().reset();
}

void free_job_workers_lib() {
  vk::singleton<PendingJobs>::get().reset();
}

void global_init_job_workers_lib() {
  if (vk::singleton<JobWorkersContext>::get().job_workers_num == 0) {
    return;
  }
  SharedContext::get();
  vk::singleton<SharedMemoryManager>::get().init();
}

void process_job_worker_answer_event(int ready_job_id, void *job_result_script_memory_ptr) {
  vk::singleton<PendingJobs>::get().mark_job_ready(ready_job_id, job_result_script_memory_ptr);
}

int64_t f$async_x2(const array<int64_t> &arr) {
  if (!f$is_job_workers_enabled()) {
    php_warning("Can't send job: job workers disabled");
    return -1;
  }
  auto &memory_manager = vk::singleton<SharedMemoryManager>::get();
  void * const memory_slice = memory_manager.allocate_slice();
  if (memory_slice == nullptr) {
    php_warning("Can't allocate slice for job: not enough shared memory");
    SharedContext::get().total_errors_shared_memory_limit++;
    return -1;
  }

  auto *job_memory = reinterpret_cast<int64_t *>(memory_slice);
  *job_memory++ = arr.count();
  memcpy(job_memory, arr.get_const_vector_pointer(), arr.count() * sizeof(int64_t));

  int job_id = vk::singleton<JobWorkerClient>::get().send_job(memory_slice);
  if (job_id <= 0) {
    memory_manager.deallocate_slice(memory_slice);
    php_warning("Can't send job. Probably jobs queue is full");
    return -1;
  }

  vk::singleton<PendingJobs>::get().put_job(job_id);
  return job_id;
}

array<int64_t> f$await_x2(int64_t job_id) {
  if (!f$is_job_workers_enabled()) {
    php_warning("Can't wait job: job workers disabled");
    return {};
  }
  auto &pending_jobs = vk::singleton<PendingJobs>::get();
  if (!pending_jobs.job_exists(job_id)) {
    php_warning("Job with id %" PRIi64 " doesn't exist", job_id);
    return {};
  }
  while (!pending_jobs.is_job_ready(job_id)) {
    wait_net(0);
  }
  return pending_jobs.withdraw_job(job_id);
}

bool f$is_job_workers_enabled() {
  return vk::singleton<JobWorkersContext>::get().job_workers_num > 0;
}
