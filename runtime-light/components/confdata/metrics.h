// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <utility>

#include "runtime-light/components/confdata/confdata-proxy/sync-functions.h"
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

struct update_failures final {
  std::array<kphp::diagnostics::metric_sender, 5> m_failures{
      kphp::diagnostics::metric_sender::metric("k2_kphp_confdata_update_fails").tag("error", "transport"),
      kphp::diagnostics::metric_sender::metric("k2_kphp_confdata_update_fails").tag("error", "old_offset"),
      kphp::diagnostics::metric_sender::metric("k2_kphp_confdata_update_fails").tag("error", "malformed_response"),
      kphp::diagnostics::metric_sender::metric("k2_kphp_confdata_update_fails").tag("error", "not_synced"),
      kphp::diagnostics::metric_sender::metric("k2_kphp_confdata_update_fails").tag("error", "batch_rejected"),
  };
  static_assert(std::to_underlying(kphp::confdata::subscribe_error::transport) == 0);
  static_assert(std::to_underlying(kphp::confdata::subscribe_error::old_offset) == 1);
  static_assert(std::to_underlying(kphp::confdata::subscribe_error::malformed_response) == 2);
  static_assert(std::to_underlying(kphp::confdata::subscribe_error::not_synced) == 3);
  static_assert(std::to_underlying(kphp::confdata::subscribe_error::batch_rejected) == 4);

  kphp::diagnostics::metric_sender m_old_offset{
      kphp::diagnostics::metric_sender::metric("k2_kphp_confdata_update_fails_old_offset"),
  };
};

} // namespace kphp::confdata::metrics
