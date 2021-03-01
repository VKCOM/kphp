// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "common/smart_ptrs/singleton.h"
#include "common/mixin/not_copyable.h"
#include "runtime/kphp_core.h"

namespace job_workers {

class PendingJobs : vk::not_copyable {
public:
  friend class vk::singleton<PendingJobs>;

  bool job_exists(int job_id) const;
  bool is_job_ready(int job_id) const;

  void put_job(int job_id);
  void mark_job_ready(int job_id, void *job_result_script_memory_ptr);
  array<int64_t> withdraw_job(int job_id);

  void reset();

private:
  array<Optional<array<int64_t>>> jobs_;

  PendingJobs() = default;
};

} // namespace job_workers
