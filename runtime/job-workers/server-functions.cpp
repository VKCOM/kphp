// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/job-workers/shared-memory-manager.h"
#include "runtime/instance-copy-processor.h"
#include "server/job-workers/simple-php-script.h"

#include "runtime/job-workers/server-functions.h"

bool f$is_job_worker_mode() noexcept {
  return job_workers::fetch_request();
}

class_instance<C$KphpJobWorkerRequest> f$job_worker_fetch_request() noexcept {
  if (!f$is_job_workers_enabled()) {
    php_warning("Can't fetch job: job workers disabled");
    return {};
  }

  job_workers::SharedMemorySlice *request = job_workers::fetch_request();
  php_assert(request);
  auto result = request->instance.cast_to<C$KphpJobWorkerRequest>();
  php_assert(!result.is_null());
  return result;
}

void f$job_worker_store_response(const class_instance<C$KphpJobWorkerReply> &response) noexcept {
  if (!f$is_job_workers_enabled()) {
    php_warning("Can't store job response: job workers disabled");
    return;
  }

  job_workers::SharedMemorySlice *response_memory = vk::singleton<job_workers::SharedMemoryManager>::get().acquire_slice();
  if (!response_memory) {
    php_warning("Can't store job response: not enough shared memory");
    return;
  }

  response_memory->instance = copy_instance_into_other_memory(response, response_memory->resource, ExtraRefCnt::for_job_worker_communication);
  job_workers::store_reply(response_memory);
}
