// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <chrono>

#include "common/algorithms/clamp.h"

#include "runtime/critical_section.h"
#include "runtime/instance-copy-processor.h"
#include "runtime/job-workers/job-interface.h"
#include "runtime/job-workers/processing-jobs.h"
#include "runtime/resumable.h"

#include "server/job-workers/job-message.h"
#include "server/job-workers/job-worker-client.h"
#include "server/job-workers/job-stats.h"
#include "server/job-workers/shared-memory-manager.h"
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

namespace {

template<typename JobMessageT, typename T>
JobMessageT *make_job_request_message(const class_instance<T> &instance) {
  auto &memory_manager = vk::singleton<job_workers::SharedMemoryManager>::get();
  auto *memory_request = memory_manager.acquire_shared_message<JobMessageT>();
  if (memory_request == nullptr) {
    php_warning("Can't send job: not enough shared memory");
    return nullptr;
  }

  memory_request->instance = copy_instance_into_other_memory(instance, memory_request->resource,
                                                             ExtraRefCnt::for_job_worker_communication, job_workers::request_extra_shared_memory);
  if (memory_request->instance.is_null()) {
    memory_manager.release_shared_message(memory_request);
    php_warning("Can't send job: too big request");
    return nullptr;
  }
  return memory_request;
}

int send_job_request_message(job_workers::JobSharedMessage *job_message, double timeout, job_workers::JobSharedMemoryPiece *parent_job = nullptr) {
  auto &client = vk::singleton<job_workers::JobWorkerClient>::get();

  const auto now = std::chrono::system_clock::now();
  job_message->job_start_time = std::chrono::duration<double>{now.time_since_epoch()}.count();
  job_message->job_timeout = timeout;

  int job_id = 0;
  {
    dl::CriticalSectionSmartGuard critical_section;
    if (parent_job) {
      job_message->bind_parent_job(parent_job);
    }
    job_id = client.send_job(job_message);
    auto &memory_manager = vk::singleton<job_workers::SharedMemoryManager>::get();
    if (job_id > 0) {
      memory_manager.detach_shared_message_from_this_proc(job_message);
    } else {
      if (parent_job) {
        job_message->unbind_parent_job();
      }
      memory_manager.release_shared_message(job_message);
      critical_section.leave_critical_section();
      php_warning("Can't send job: probably jobs queue is full");
      return -1;
    }
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
  auto &client = vk::singleton<job_workers::JobWorkerClient>::get();
  if (!client.is_inited()) {
    php_warning("Can't send job: call from job worker");
    return false;
  }
  return true;
}

double normalize_job_timeout(double timeout) {
  if (timeout < 0) {
    return DEFAULT_SCRIPT_TIMEOUT;
  } else if (timeout < 0.05 || timeout > MAX_SCRIPT_TIMEOUT) {
    php_warning("timeout value was clamped to [%f; %d]", 0.05, MAX_SCRIPT_TIMEOUT);
    return vk::clamp(timeout, 0.05, MAX_SCRIPT_TIMEOUT * 1.0);
  }
  return timeout;
}

} // namespace

Optional<int64_t> f$kphp_job_worker_start(const class_instance<C$KphpJobWorkerRequest> &request, double timeout) noexcept {
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

  int job_resumable_id = send_job_request_message(memory_request, timeout);

  if (job_resumable_id < 0) {
    return false;
  }
  
  return job_resumable_id;
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

  job_workers::JobSharedMemoryPiece *parent_job_request = nullptr;
  if (!common_shared_memory_piece.is_null()) {
    parent_job_request = make_job_request_message<job_workers::JobSharedMemoryPiece>(common_shared_memory_piece);
    /**
     * parent_job_request lifetime:
     * 1. attaches to client process on creating in `acquire_shared_message()`
     * 2. attaches to job worker process on job receiving in `job_parse_execute()`
     * 3. detaches from job worker process on terminating in `release_shared_message()` recursively
     * 4. detaches from client process at the end of this function in `release_shared_message()` recursively
     *
     * client: 1 -> 4
     *         ↓
     * server: 2 -> 3
     */
    if (parent_job_request == nullptr) {
      return {};
    }
  }

  for (const auto &it : requests) {
    const auto &req = it.get_value();

    req.get()->set_shared_memory_piece({});                                                 // prepare for copying to shared memory
    auto *child_job_request = make_job_request_message<job_workers::JobSharedMessage>(req); // copy to shared memory
    req.get()->set_shared_memory_piece(common_shared_memory_piece);                         // roll it back to keep original instance unchanged
    if (child_job_request == nullptr) {
      res.set_value(it.get_key(), false);
      continue;
    }

    if (parent_job_request) {
      const auto &child_instance = child_job_request->instance.cast_to<C$KphpJobWorkerRequest>();
      const auto &parent_instance = parent_job_request->instance.cast_to<C$KphpJobWorkerSharedMemoryPiece>();
      php_assert(!child_instance.is_null());
      php_assert(!parent_instance.is_null());
      child_instance.get()->set_shared_memory_piece(parent_instance);
    }

    int job_resumable_id = send_job_request_message(child_job_request, timeout, parent_job_request);
    if (job_resumable_id > 0) {
      res.set_value(it.get_key(), job_resumable_id);
    } else {
      res.set_value(it.get_key(), false);
    }
  }

  if (parent_job_request) {
    vk::singleton<job_workers::SharedMemoryManager>::get().release_shared_message(parent_job_request);
  }

  return res;
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
