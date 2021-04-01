// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <chrono>

#include "common/algorithms/clamp.h"

#include "runtime/instance-copy-processor.h"
#include "runtime/job-workers/job-interface.h"
#include "runtime/job-workers/job-message.h"
#include "runtime/job-workers/processing-jobs.h"
#include "runtime/job-workers/shared-memory-manager.h"
#include "runtime/resumable.h"

#include "server/job-workers/job-worker-client.h"
#include "server/job-workers/job-stats.h"
#include "server/php-engine-vars.h"

#include "runtime/job-workers/client-functions.h"

class job_resumable : public Resumable {
public:
  using ReturnT = class_instance<C$KphpJobWorkerResponse>;

  explicit job_resumable(int job_id)
    : job_id(job_id) {}

protected:
  bool run() final {
    const class_instance<C$KphpJobWorkerResponse> &res = vk::singleton<job_workers::ProcessingJobs>::get().withdraw(job_id);
    RETURN(res);
  }
private:
  int job_id;
};

class kphp_job_worker_wait_resumable : public Resumable {
public:
  using ReturnT = class_instance<C$KphpJobWorkerResponse>;

  kphp_job_worker_wait_resumable(int64_t resumable_id, double timeout)
    : resumable_id(resumable_id)
    , timeout(timeout) {}

protected:
  bool run() final {
    bool ready{false};

    RESUMABLE_BEGIN
      ready = wait_without_result(resumable_id, timeout);
      TRY_WAIT(kphp_job_worker_wait_resumable_label_0, ready, bool);
      if (!ready) {
        RETURN({});
      }

      Storage *input = get_forked_storage(resumable_id);
      if (input->tag == 0) {
        php_warning("Job result already was gotten\n");
        RETURN({});
      }
      if (input->tag != Storage::tagger<ReturnT>::get_tag()) {
        php_warning("Not a job request\n");
        RETURN({});
      }

      const ReturnT &res = input->load<ReturnT>();
      php_assert (CurException.is_null());

      RETURN(res);
    RESUMABLE_END
  }
private:
  int64_t resumable_id;
  double timeout;
};

int64_t f$kphp_job_worker_start(const class_instance<C$KphpJobWorkerRequest> &request, double timeout) noexcept {
  if (!f$is_kphp_job_workers_enabled()) {
    php_warning("Can't send job: job workers disabled");
    return -1;
  }
  if (request.is_null()) {
    php_warning("Can't send job: the request shouldn't be null");
    return -1;
  }
  if (timeout < 0) {
    timeout = DEFAULT_SCRIPT_TIMEOUT;
  } else if (timeout < 1 || timeout > MAX_SCRIPT_TIMEOUT) {
    timeout = vk::clamp(timeout, 1.0, MAX_SCRIPT_TIMEOUT * 1.0);
    php_warning("timeout value was clamped to [%d; %d]", 1, MAX_SCRIPT_TIMEOUT);
  }

  auto &memory_manager = vk::singleton<job_workers::SharedMemoryManager>::get();
  auto *memory_request = memory_manager.acquire_shared_message();
  if (memory_request == nullptr) {
    php_warning("Can't send job: not enough shared memory");
    return -1;
  }

  memory_request->instance = copy_instance_into_other_memory(request, memory_request->resource, ExtraRefCnt::for_job_worker_communication);
  if (memory_request->instance.is_null()) {
    memory_manager.release_shared_message(memory_request);
    php_warning("Can't send job: too big request");
    return -1;
  }

  auto now = std::chrono::system_clock::now();
  double job_deadline_time = std::chrono::duration_cast<std::chrono::duration<double>>(now.time_since_epoch()).count() + timeout;
  memory_request->job_deadline_time = job_deadline_time;

  const int job_id = vk::singleton<job_workers::JobWorkerClient>::get().send_job(memory_request);
  if (job_id <= 0) {
    memory_manager.release_shared_message(memory_request);
    php_warning("Can't send job: probably jobs queue is full");
    return -1;
  }

  int64_t job_resumable_id = register_forked_resumable(new job_resumable{job_id});

  update_precise_now();
  kphp_event_timer *timer = allocate_event_timer(get_precise_now() + timeout, get_job_timeout_wakeup_id(), job_id);

  vk::singleton<job_workers::ProcessingJobs>::get().start_job_processing(job_id, job_workers::JobRequestInfo{job_resumable_id, timer});
  return job_resumable_id;
}

class_instance<C$KphpJobWorkerResponse> f$kphp_job_worker_wait(int64_t job_resumable_id, double timeout) noexcept {
  if (!f$is_kphp_job_workers_enabled()) {
    php_warning("Can't wait job: job workers disabled");
    return {};
  }
  return start_resumable<class_instance<C$KphpJobWorkerResponse>>(new kphp_job_worker_wait_resumable(job_resumable_id, timeout));
}

void free_job_client_interface_lib() noexcept {
  if (f$is_kphp_job_workers_enabled()) {
    vk::singleton<job_workers::ProcessingJobs>::get().reset();
  }
}
