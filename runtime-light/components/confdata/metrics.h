// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>

#include "runtime-light/stdlib/diagnostics/metrics.h"

namespace kphp::confdata::metrics {

// Each instance constructs these builders once, after its script allocator is ready.
struct capacity final {
  std::array<kphp::diagnostics::metric_builder, 4> m_memory{
      kphp::diagnostics::metric_builder::metric("k2_confdata_memory_bytes").tag("kind", "limit"),
      kphp::diagnostics::metric_builder::metric("k2_confdata_memory_bytes").tag("kind", "used"),
      kphp::diagnostics::metric_builder::metric("k2_confdata_memory_bytes").tag("kind", "real_used"),
      kphp::diagnostics::metric_builder::metric("k2_confdata_memory_bytes").tag("kind", "oom_threshold"),
  };
  std::array<kphp::diagnostics::metric_builder, 2> m_pieces{
      kphp::diagnostics::metric_builder::metric("k2_confdata_pieces").tag("state", "current"),
      kphp::diagnostics::metric_builder::metric("k2_confdata_pieces").tag("state", "retired"),
  };
};

} // namespace kphp::confdata::metrics
