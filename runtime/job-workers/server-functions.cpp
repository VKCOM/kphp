// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/critical_section.h"
#include "runtime/instance-copy-processor.h"

#include "server/job-workers/job-message.h"
#include "server/job-workers/shared-memory-manager.h"
#include "server/php-query-data.h"

#include "runtime/job-workers/server-functions.h"

static job_query_data current_job;

void init_job_server_interface_lib(job_query_data job_data) noexcept {
  php_assert(!current_job.job_request);
  php_assert(!current_job.send_reply);

  current_job = job_data;
}

void free_job_server_interface_lib() noexcept {
  if (current_job.job_request) {
    vk::singleton<job_workers::SharedMemoryManager>::get().release_shared_message(current_job.job_request);
  }

  current_job = job_query_data{nullptr, nullptr};
}

class_instance<C$KphpJobWorkerRequest> f$kphp_job_worker_fetch_request() noexcept {
  if (!f$is_kphp_job_workers_enabled()) {
    php_warning("Can't fetch job: job workers disabled");
    return {};
  }
  if (!current_job.job_request) {
    php_warning("Can't fetch job: there is no job requests");
    return {};
  }
  auto result = current_job.job_request->instance.cast_to<C$KphpJobWorkerRequest>();
  php_assert(!result.is_null());
  return result;
}

int64_t f$kphp_job_worker_store_response(const class_instance<C$KphpJobWorkerResponse> &response) noexcept {
  if (response.is_null()) {
    php_warning("Can't store job response: the response shouldn't be null");
    return job_workers::store_response_incorrect_call_error;
  }
  if (!f$is_kphp_job_workers_enabled()) {
    php_warning("Can't store job response %s: job workers disabled", response.get_class());
    return job_workers::store_response_incorrect_call_error;
  }
  if (!current_job.send_reply) {
    php_warning("Can't store job response %s: this is a not job request", response.get_class());
    return job_workers::store_response_incorrect_call_error;
  }
  auto &memory_manager = vk::singleton<job_workers::SharedMemoryManager>::get();
  auto *response_memory = memory_manager.acquire_shared_message<job_workers::JobSharedMessage>();
  if (!response_memory) {
    php_warning("Can't store job response %s: not enough shared messages", response.get_class());
    return job_workers::store_response_not_enough_shared_messages_error;
  }
  response_memory->instance = copy_instance_into_other_memory(response, response_memory->resource,
                                                              ExtraRefCnt::for_job_worker_communication, job_workers::request_extra_shared_memory);
  if (response_memory->instance.is_null()) {
    php_warning("Can't store job response %s: too big response", response.get_class());
    memory_manager.release_shared_message(response_memory);
    return job_workers::store_response_too_big_error;
  }

  dl::CriticalSectionSmartGuard critical_section;
  if (const char *err = current_job.send_reply(response_memory)) {
    memory_manager.release_shared_message(response_memory);
    critical_section.leave_critical_section();
    php_warning("Can't store job response %s: %s", response.get_class(), err);
    return job_workers::store_response_cant_send_error;
  } else {
    memory_manager.detach_shared_message_from_this_proc(response_memory);
    return 0;
  }
}
