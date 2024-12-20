// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "common/mixin/not_copyable.h"

namespace kphp_tracing {

struct TracingContext final : private vk::not_copyable {
  // cur_trace_level is level of tracing for current php script
  // -1 - not inited, disabled (on php script finish, it's reset to -1)
  // 0  - inited, but turned off
  //      (binlog is not written, on_rpc_query_send() and other "callbacks" are not called)
  // 1  - turned on, this php script is traced in a regular mode
  //      (binlog is written, and when the script finishes, it's either flushed to a file or dismissed)
  // 2  - turned on in advanced mode
  //      (same as 1, but more events with more details are written to binlog, significant overhead)
  int32_t cur_trace_level{-1};

  static TracingContext& get() noexcept;
};

} // namespace kphp_tracing
