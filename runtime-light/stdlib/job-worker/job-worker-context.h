// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "common/mixin/not_copyable.h"
#include "runtime-core/runtime-core.h"
#include "runtime-light/stdlib/job-worker/job-worker.h"

struct JobWorkerServerComponentContext final : private vk::not_copyable {
  enum class Kind : uint8_t { Invalid, Regular, NoReply };

  Kind kind{Kind::Invalid};
  int64_t job_id{JOB_WORKER_INVALID_JOB_ID};
  string body;

  static JobWorkerServerComponentContext &get() noexcept;
};

struct JobWorkerClientComponentContext final : private vk::not_copyable {
  int64_t current_job_id{JOB_WORKER_VALID_JOB_ID_RANGE_START};

  static JobWorkerClientComponentContext &get() noexcept;
};
