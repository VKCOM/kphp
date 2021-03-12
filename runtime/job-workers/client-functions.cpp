// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/job-workers/processing-jobs.h"
#include "runtime/job-workers/shared-memory-manager.h"
#include "runtime/instance-copy-processor.h"
#include "runtime/net_events.h"

#include "server/job-workers/job-worker-client.h"
#include "server/job-workers/job-stats.h"

#include "runtime/job-workers/client-functions.h"

int64_t f$kphp_job_worker_start(const class_instance<C$KphpJobWorkerRequest> &request) noexcept {
  if (request.is_null()) {
    php_warning("Can't send job: the request shouldn't be null");
    return -1;
  }
  if (!f$is_kphp_job_workers_enabled()) {
    php_warning("Can't send job: job workers disabled");
    return -1;
  }

  auto &memory_manager = vk::singleton<job_workers::SharedMemoryManager>::get();
  job_workers::SharedMemorySlice *memory_request = memory_manager.acquire_slice();
  if (memory_request == nullptr) {
    php_warning("Can't send job: not enough shared memory");
    return -1;
  }

  memory_request->instance = copy_instance_into_other_memory(request, memory_request->resource, ExtraRefCnt::for_job_worker_communication);
  if (memory_request->instance.is_null()) {
    memory_manager.release_slice(memory_request);
    php_warning("Can't send job: too big request");
    return -1;
  }

  const int job_id = vk::singleton<job_workers::JobWorkerClient>::get().send_job(memory_request);
  if (job_id <= 0) {
    memory_manager.release_slice(memory_request);
    php_warning("Can't send job: probably jobs queue is full");
    return -1;
  }

  vk::singleton<job_workers::ProcessingJobs>::get().start_job_processing(job_id);
  return job_id;
}

class_instance<C$KphpJobWorkerResponse> f$kphp_job_worker_wait(int64_t job_id) noexcept {
  if (!f$is_kphp_job_workers_enabled()) {
    php_warning("Can't wait job: job workers disabled");
    return {};
  }
  auto &processing_jobs = vk::singleton<job_workers::ProcessingJobs>::get();
  if (!processing_jobs.is_started(job_id)) {
    php_warning("Job with id %" PRIi64 " doesn't exist", job_id);
    return {};
  }
  while (!processing_jobs.is_ready(job_id)) {
    wait_net(0);
  }
  auto response = processing_jobs.withdraw(job_id);
  // TODO check response for OOM?
  php_assert(response.is_null() || response.get_reference_counter() == 1);
  return response;
}

void free_job_client_interface_lib() noexcept {
  if (f$is_kphp_job_workers_enabled()) {
    vk::singleton<job_workers::ProcessingJobs>::get().reset();
  }
}
