// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-core/runtime-core.h"
#include "runtime-light/tl/tl-core.h"

namespace tl {

inline constexpr uint32_t K2_INVOKE_HTTP_MAGIC = 0xd909efe8;
inline constexpr uint32_t K2_INVOKE_JOB_WORKER_MAGIC = 0x437d7312;

struct K2InvokeJobWorker final {
  uint64_t image_id{};
  int64_t job_id{};
  bool ignore_answer{};
  uint64_t timeout_ns{};
  string body;

  bool fetch(TLBuffer &tlb) noexcept;

  void store(TLBuffer &tlb) const noexcept;
};

} // namespace tl
