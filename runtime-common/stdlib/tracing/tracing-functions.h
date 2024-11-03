// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/stdlib/tracing/tracing-context.h"

namespace kphp_tracing {

inline bool is_turned_on() {
  return TracingContext::get().cur_trace_level >= 1;
}

} // namespace kphp_tracing
