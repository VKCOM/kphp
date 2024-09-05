// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-core/runtime-core.h"
#include "runtime-light/tl/tl-core.h"

namespace tl {

constexpr uint32_t K2_JOB_WORKER_RESPONSE_MAGIC = 0x3afb3a08;

struct K2JobWorkerResponse {
  uint32_t flags{};
  int64_t job_id{};
  string body;

  bool fetch(TLBuffer &tlb) noexcept;

  void store(TLBuffer &tlb) const noexcept;
};

} // namespace tl
