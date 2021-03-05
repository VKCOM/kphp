// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"

#include "runtime/kphp_core.h"

#include "runtime/job-workers/job-interface.h"

namespace job_workers {

struct SharedMemorySlice;

class ProcessingJobs : vk::not_copyable {
public:
  void start_job_processing(int job_slot_id) noexcept {
    processing_[job_slot_id] = ProcessingJobAwait{};
  }

  bool is_started(int job_slot_id) const noexcept {
    return processing_.has_key(job_slot_id);
  }

  void finish_job_processing(int job_slot_id, SharedMemorySlice *reply_slice) noexcept;
  bool is_ready(int job_slot_id) const noexcept;
  class_instance<C$KphpJobWorkerReply> withdraw(int job_slot_id) noexcept;

  void reset() noexcept {
    hard_reset_var(processing_);
  }

private:
  ProcessingJobs() = default;

  friend class vk::singleton<ProcessingJobs>;

  struct ProcessingJobAwait {
    class_instance<C$KphpJobWorkerReply> reply;
    bool ready{false};
  };
  array<ProcessingJobAwait> processing_;
};

} // namespace job_workers
