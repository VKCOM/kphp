// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <algorithm>
#include <chrono>

#include "runtime/critical_section.h"
#include "runtime/instance-copy-processor.h"
#include "runtime/job-workers/job-interface.h"
#include "runtime/job-workers/processing-jobs.h"
#include "runtime/kphp_tracing.h"
#include "runtime/resumable.h"

#include "server/job-workers/job-message.h"
#include "server/job-workers/job-worker-client.h"
#include "server/job-workers/job-stats.h"
#include "server/job-workers/shared-memory-manager.h"
#include "server/php-engine-vars.h"
#include "server/server-stats.h"
#include "server/slot-ids-factory.h"

#include "runtime/job-workers/client-functions.h"

class job_resumable : public Resumable {
public:
  using ReturnT = class_instance<C$KphpJobWorkerResponse>;

  explicit job_resumable(int job_id)
    : job_id(job_id) {}

  bool is_internal_resumable() const noexcept final {
    return true;
  }

protected:
  bool run() final {
    const class_instance<C$KphpJobWorkerResponse> &res = vk::singleton<job_workers::ProcessingJobs>::get().withdraw(job_id);
    RETURN(res);
  }
private:
  int job_id;
};

namespace {

template<typename JobMessageT, typename T>
JobMessageT *make_job_request_message(const class_instance<T> &instance) {
  auto &memory_manager = vk::singleton<job_workers::SharedMemoryManager>::get();
  auto *memory_request = memory_manager.acquire_shared_message<JobMessageT>();
  if (memory_request == nullptr) {
    php_notice("Can't send job %s: not enough shared messages. "
               "Most probably job workers are slowed and overloaded due to external factors: net/cpu lags, network queries slowdown etc.", instance.get_class());
    return nullptr;
  }

  memory_request->instance = copy_instance_into_other_memory(instance, memory_request->resource,
                                                             ExtraRefCnt::for_job_worker_communication, job_workers::request_extra_shared_memory);
  if (memory_request->instance.is_null()) {
    memory_manager.release_shared_message(memory_request);
    php_warning("Can't send job %s: too big request", instance.get_class());
    return nullptr;
  }
  return memory_request;
}

void init_job_request_metadata(job_workers::JobSharedMessage *job_message, bool no_reply, double timeout) {
  const auto now = std::chrono::system_clock::now();

  job_message->no_reply = no_reply;
  job_message->job_timeout = timeout;
  job_message->job_id = parallel_job_ids_factory.create_slot();
  job_message->job_start_time = std::chrono::duration<double>{now.time_since_epoch()}.count();
}

int send_job_request_message(job_workers::JobSharedMessage *job_message, double timeout, job_workers::JobSharedMemoryPiece *common_job = nullptr, bool no_reply = false) {
  auto &client = vk::singleton<job_workers::JobWorkerClient>::get();

  // save it here, as it's incorrect to use job_message after send
  int job_id = job_message->job_id;

  {
    dl::CriticalSectionSmartGuard critical_section;
    if (common_job) {
      job_message->bind_common_job(common_job);
    }
    bool success = client.send_job(job_message);
    auto &memory_manager = vk::singleton<job_workers::SharedMemoryManager>::get();
    if (success) {
      memory_manager.detach_shared_message_from_this_proc(job_message);
    } else {
      if (common_job) {
        job_message->unbind_common_job();
      }
      memory_manager.release_shared_message(job_message);
      critical_section.leave_critical_section();
      php_warning("Can't send job %s: probably jobs queue is full", job_message->instance.get_class());
      return -1;
    }
  }

  if (no_reply) {
    return 0;
  }

  int64_t job_resumable_id = register_forked_resumable(new job_resumable{job_id});

  update_precise_now();
  kphp_event_timer *timer = allocate_event_timer(get_precise_now() + timeout, get_job_timeout_wakeup_id(), job_id);

  vk::singleton<job_workers::ProcessingJobs>::get().start_job_processing(job_id, job_workers::JobRequestInfo{job_resumable_id, timer});
  
  return job_resumable_id;
}

bool job_workers_api_allowed() {
  if (!f$is_kphp_job_workers_enabled()) {
    php_warning("Can't send job: job workers disabled");
    return false;
  }
  return true;
}

double normalize_job_timeout(double timeout) {
  if (timeout < 0) {
    return script_timeout;
  }
  if (timeout < 0.05 || timeout > script_timeout) {
    php_warning("timeout value was clamped to [%f; %d]", 0.05, script_timeout);
    return std::clamp(timeout, 0.05, script_timeout * 1.0);
  }
  return timeout;
}

Optional<int64_t> kphp_job_worker_start_impl(const class_instance<C$KphpJobWorkerRequest> &request, double timeout, bool no_reply) noexcept {
  if (!job_workers_api_allowed()) {
    return false;
  }
  if (request.is_null()) {
    php_warning("Can't send job: the request shouldn't be null");
    return false;
  }
  timeout = normalize_job_timeout(timeout);

  auto *memory_request = make_job_request_message<job_workers::JobSharedMessage>(request);
  if (memory_request == nullptr) {
    return false;
  }
  init_job_request_metadata(memory_request, no_reply, timeout);

  int job_id = memory_request->job_id;
  double job_start_time = memory_request->job_start_time;

  int job_resumable_id = send_job_request_message(memory_request, timeout, nullptr, no_reply);

  if (kphp_tracing::is_turned_on()) {
    kphp_tracing::on_job_worker_start(job_id, f$get_class(request), job_start_time, no_reply);
  }

  if (job_resumable_id < 0) {
    return false;
  }

  return job_resumable_id;
}

} // namespace

Optional<int64_t> f$kphp_job_worker_start(const class_instance<C$KphpJobWorkerRequest> &request, double timeout) noexcept {
  return kphp_job_worker_start_impl(request, timeout, false);
}

bool f$kphp_job_worker_start_no_reply(const class_instance<C$KphpJobWorkerRequest> &request, double timeout) noexcept {
  return kphp_job_worker_start_impl(request, timeout, true).has_value();
}

array<Optional<int64_t>> f$kphp_job_worker_start_multi(const array<class_instance<C$KphpJobWorkerRequest>> &requests, double timeout) noexcept {
  if (!job_workers_api_allowed()) {
    return {};
  }
  timeout = normalize_job_timeout(timeout);

  class_instance<C$KphpJobWorkerSharedMemoryPiece> common_shared_memory_piece;
  bool first = true;
  for (const auto &it : requests) {
    const auto &req = it.get_value();
    
    if (req.is_null()) {
      php_warning("Can't send multiple jobs: requests[%s] is null", it.get_key().to_string().c_str());
      return {};
    }
    
    const auto &cur_shared_mem_piece = req.get()->get_shared_memory_piece();
    if (first) {
      common_shared_memory_piece = cur_shared_mem_piece; // increment ref count here to prevent unexpected destroy on temporary resetting before copying below
      first = false;
    } else {
      if (cur_shared_mem_piece.get() != common_shared_memory_piece.get()) {
        php_warning("Can't extract common shared memory piece in multiple jobs request: requests[%s] and requests[%s] have different shared memory pieces",
                    requests.begin().get_key().to_string().c_str(), it.get_key().to_string().c_str());
        return {};
      }
    }
  }

  array<Optional<int64_t>> res{requests.size()};

  job_workers::JobSharedMemoryPiece *common_job_request = nullptr;
  if (!common_shared_memory_piece.is_null()) {
    common_job_request = make_job_request_message<job_workers::JobSharedMemoryPiece>(common_shared_memory_piece);
    /**
     * common_job_request lifetime:
     * 1. attaches to client process on creating in `acquire_shared_message()`
     * 2. attaches to job worker process on job receiving in `job_parse_execute()`
     * 3. detaches from job worker process on terminating in `release_shared_message()` recursively
     * 4. detaches from client process at the end of this function in `release_shared_message()`
     *
     * client: 1 -> 4
     *         ↓
     * server: 2 -> 3
     */
    if (common_job_request == nullptr) {
      return {};
    }
    const auto &job_mem_stats = common_job_request->resource.get_memory_stats();
    vk::singleton<ServerStats>::get().add_job_common_memory_stats(job_mem_stats.max_memory_used, job_mem_stats.max_real_memory_used);
  }

  for (const auto &it : requests) {
    const auto &req = it.get_value();

    req.get()->set_shared_memory_piece({});                                           // prepare for copying to shared memory
    auto *job_request = make_job_request_message<job_workers::JobSharedMessage>(req); // copy to shared memory
    req.get()->set_shared_memory_piece(common_shared_memory_piece);                   // roll it back to keep original instance unchanged
    if (job_request == nullptr) {
      res.set_value(it.get_key(), false);
      continue;
    }

    if (common_job_request) {
      const auto &job_instance = job_request->instance.cast_to<C$KphpJobWorkerRequest>();
      const auto &common_job_instance = common_job_request->instance.cast_to<C$KphpJobWorkerSharedMemoryPiece>();
      php_assert(!job_instance.is_null());
      php_assert(!common_job_instance.is_null());
      job_instance.get()->set_shared_memory_piece(common_job_instance);
    }
    init_job_request_metadata(job_request, false, timeout);

    int job_id = job_request->job_id;
    double job_start_time = job_request->job_start_time;

    int job_resumable_id = send_job_request_message(job_request, timeout, common_job_request);

    if (kphp_tracing::is_turned_on()) {
      kphp_tracing::on_job_worker_start(job_id, f$get_class(req), job_start_time, false);
    }

    if (job_resumable_id > 0) {
      res.set_value(it.get_key(), job_resumable_id);
    } else {
      res.set_value(it.get_key(), false);
    }
  }

  if (common_job_request) {
    vk::singleton<job_workers::SharedMemoryManager>::get().release_shared_message(common_job_request);
  }

  return res;
}

void free_job_client_interface_lib() noexcept {
  if (f$is_kphp_job_workers_enabled()) {
    vk::singleton<job_workers::ProcessingJobs>::get().reset();
  }
}
