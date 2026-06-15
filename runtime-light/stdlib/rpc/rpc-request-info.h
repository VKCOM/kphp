// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>

#include "runtime-light/k2-platform/k2-api.h"

namespace kphp::rpc {

struct request_info {
  k2::descriptor rpc_d;
  std::chrono::nanoseconds deadline;
  bool collect_responses_extra_info;
};

} // namespace kphp::rpc
