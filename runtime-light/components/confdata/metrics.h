// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>

#include "runtime-light/stdlib/diagnostics/metrics.h"

namespace kphp::confdata::metrics {

// Each instance constructs these builders once, after its script allocator is ready.
struct capacity final {
  std::array<kphp::diagnostics::metric_sender, 4> m_memory{
      kphp::diagnostics::metric_sender::metric("k2_kphp_confdata_memory").tag("kind", "limit"),
      kphp::diagnostics::metric_sender::metric("k2_kphp_confdata_memory").tag("kind", "used"),
      kphp::diagnostics::metric_sender::metric("k2_kphp_confdata_memory").tag("kind", "real_used"),
      kphp::diagnostics::metric_sender::metric("k2_kphp_confdata_memory").tag("kind", "oom_threshold"),
  };
  std::array<kphp::diagnostics::metric_sender, 2> m_pieces{
      kphp::diagnostics::metric_sender::metric("k2_kphp_confdata_pieces").tag("state", "current"),
      kphp::diagnostics::metric_sender::metric("k2_kphp_confdata_pieces").tag("state", "retired"),
  };
};

struct events final {
  std::array<kphp::diagnostics::metric_sender, 2> m_events{
      kphp::diagnostics::metric_sender::metric("k2_kphp_confdata_events").tag("kind", "update"),
      kphp::diagnostics::metric_sender::metric("k2_kphp_confdata_events").tag("kind", "delete"),
  };
};

} // namespace kphp::confdata::metrics
