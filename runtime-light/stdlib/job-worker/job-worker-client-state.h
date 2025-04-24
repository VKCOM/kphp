// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "common/mixin/not_copyable.h"
#include "runtime-light/stdlib/job-worker/job-worker.h"

struct JobWorkerClientInstanceState final : private vk::not_copyable {
  int64_t current_job_id{JOB_WORKER_VALID_JOB_ID_RANGE_START};

  JobWorkerClientInstanceState() noexcept = default;

  static JobWorkerClientInstanceState& get() noexcept;
};
