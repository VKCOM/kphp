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
  // The families are declared before the sender arrays: the senders bind their series on initialization.
  kphp::diagnostics::gauge_metric m_memory_metric{kphp::diagnostics::gauge_metric::metric("k2_kphp_confdata_memory", {"kind"})};
  kphp::diagnostics::gauge_metric m_pieces_metric{kphp::diagnostics::gauge_metric::metric("k2_kphp_confdata_pieces", {"state"})};

  std::array<kphp::diagnostics::gauge_sender, 4> m_memory{
      m_memory_metric.series({{"kind", "limit"}}),
      m_memory_metric.series({{"kind", "used"}}),
      m_memory_metric.series({{"kind", "real_used"}}),
      m_memory_metric.series({{"kind", "oom_threshold"}}),
  };
  std::array<kphp::diagnostics::gauge_sender, 2> m_pieces{
      m_pieces_metric.series({{"state", "current"}}),
      m_pieces_metric.series({{"state", "retired"}}),
  };
};

struct events final {
  kphp::diagnostics::counter_metric m_events_metric{kphp::diagnostics::counter_metric::metric("k2_kphp_confdata_events", {"kind"})};

  std::array<kphp::diagnostics::counter_sender, 2> m_events{
      m_events_metric.series({{"kind", "update"}}),
      m_events_metric.series({{"kind", "delete"}}),
  };
};

struct update_failures final {
  kphp::diagnostics::counter_metric m_failures_metric{kphp::diagnostics::counter_metric::metric("k2_kphp_confdata_update_fails", {"error"})};
  kphp::diagnostics::gauge_metric m_old_offset_metric{kphp::diagnostics::gauge_metric::metric("k2_kphp_confdata_update_fails_old_offset")};

  std::array<kphp::diagnostics::counter_sender, 5> m_failures{
      m_failures_metric.series({{"error", "transport"}}),          m_failures_metric.series({{"error", "old_offset"}}),
      m_failures_metric.series({{"error", "malformed_response"}}), m_failures_metric.series({{"error", "not_synced"}}),
      m_failures_metric.series({{"error", "batch_rejected"}}),
  };
  static_assert(std::to_underlying(kphp::confdata::subscribe_error::transport) == 0);
  static_assert(std::to_underlying(kphp::confdata::subscribe_error::old_offset) == 1);
  static_assert(std::to_underlying(kphp::confdata::subscribe_error::malformed_response) == 2);
  static_assert(std::to_underlying(kphp::confdata::subscribe_error::not_synced) == 3);
  static_assert(std::to_underlying(kphp::confdata::subscribe_error::batch_rejected) == 4);

  kphp::diagnostics::gauge_sender m_old_offset{m_old_offset_metric.series({})};
};

} // namespace kphp::confdata::metrics
