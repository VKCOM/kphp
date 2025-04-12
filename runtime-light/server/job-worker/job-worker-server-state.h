// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/job-worker/job-worker.h"

struct JobWorkerServerInstanceState final : private vk::not_copyable {
  enum class Kind : uint8_t { Invalid, Regular, NoReply };
  enum class State : uint8_t { Invalid, Working, Replied };

  Kind kind{Kind::Invalid};
  State state{State::Invalid};
  int64_t job_id{JOB_WORKER_INVALID_JOB_ID};
  string body;

  static JobWorkerServerInstanceState& get() noexcept;
};
