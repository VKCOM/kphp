// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/job-workers/shared-memory-manager.h"
#include "runtime/instance-copy-processor.h"

#include "runtime/job-workers/server-functions.h"

struct {
  job_workers::SharedMemorySlice *request_memory{nullptr};
  const char *(*send_reply) (job_workers::SharedMemorySlice *) {nullptr};
} static current_job;

void init_job_server_interface_lib(job_workers::SharedMemorySlice *request_memory, const char *(*send_reply) (job_workers::SharedMemorySlice *)) noexcept {
  php_assert(!current_job.request_memory);
  php_assert(!current_job.send_reply);

  current_job.request_memory = request_memory;
  current_job.send_reply = send_reply;
}

void free_job_server_interface_lib() noexcept {
  if (current_job.request_memory) {
    vk::singleton<job_workers::SharedMemoryManager>::get().release_slice(current_job.request_memory);
  }
  current_job.request_memory = nullptr;
  current_job.send_reply = nullptr;
}

class_instance<C$KphpJobWorkerRequest> f$job_worker_fetch_request() noexcept {
  if (!f$is_job_workers_enabled()) {
    php_warning("Can't fetch job: job workers disabled");
    return {};
  }
  if (!current_job.request_memory) {
    php_warning("Can't fetch job: there is no job requests");
    return {};
  }
  auto result = current_job.request_memory->instance.cast_to<C$KphpJobWorkerRequest>();
  php_assert(!result.is_null());
  return result;
}

void f$job_worker_store_response(const class_instance<C$KphpJobWorkerReply> &response) noexcept {
  if (response.is_null()) {
    php_warning("Can't store job response: the response shouldn't be null");
    return;
  }
  if (!f$is_job_workers_enabled()) {
    php_warning("Can't store job response: job workers disabled");
    return;
  }
  if (!current_job.send_reply) {
    php_warning("Can't store job response: this is a not job request");
    return;
  }
  job_workers::SharedMemorySlice *response_memory = vk::singleton<job_workers::SharedMemoryManager>::get().acquire_slice();
  if (!response_memory) {
    php_warning("Can't store job response: not enough shared memory");
    return;
  }
  response_memory->instance = copy_instance_into_other_memory(response, response_memory->resource, ExtraRefCnt::for_job_worker_communication);
  if (response_memory->instance.is_null()) {
      php_warning("Can't store job response: too big response");
      vk::singleton<job_workers::SharedMemoryManager>::get().release_slice(response_memory);
      return;
  }
  if (const char *err = current_job.send_reply(response_memory)) {
    vk::singleton<job_workers::SharedMemoryManager>::get().release_slice(response_memory);
    php_warning("Can't store job response: %s", err);
  }
}
