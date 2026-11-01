// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <initializer_list>
#include <span>
#include <string_view>
#include <utility>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::diagnostics {

namespace impl {

inline kphp::stl::vector<k2::MetricLabelName, kphp::memory::script_allocator>
make_k2_metric_label_names(std::initializer_list<std::string_view> label_names) noexcept {
  kphp::stl::vector<k2::MetricLabelName, kphp::memory::script_allocator> k2_label_names;
  k2_label_names.reserve(label_names.size());
  for (std::string_view label_name : label_names) {
    k2_label_names.emplace_back(k2::MetricLabelName{.ptr = label_name.data(), .size = label_name.size()});
  }
  return k2_label_names;
}

inline kphp::stl::vector<k2::MetricLabel, kphp::memory::script_allocator>
make_k2_metric_labels(std::initializer_list<std::pair<std::string_view, std::string_view>> labels) noexcept {
  kphp::stl::vector<k2::MetricLabel, kphp::memory::script_allocator> k2_labels;
  k2_labels.reserve(labels.size());
  for (const auto& [name, value] : labels) {
    k2_labels.emplace_back(k2::MetricLabel{.name = {.ptr = name.data(), .size = name.size()}, .value = {.ptr = value.data(), .size = value.size()}});
  }
  return k2_labels;
}

} // namespace impl

// A metric family: the name and the label schema (the label KEYS) registered once at construction.
// Series of the family are created via `series(...)`; the label VALUES are named
// (`{{"key", "value"}, ...}`, in any order — the keys must cover the schema exactly).
// On a registration failure the error is reported once and the family
// becomes invalid — all its senders are no-ops.
//
// The senders write through a pre-bound series descriptor, so a send is a single FFI call
// without any string work. Every series is bound at component initialization, so the set
// of series of a metric is fixed after initialization.
// `timestamp` is the wall-clock time of the sample; the default `k2::SystemTime{}` means "now"
// (the k2-node substitutes the current time).

struct gauge_sender final {
private:
  k2::descriptor m_series;

  explicit gauge_sender(k2::descriptor series) noexcept
      : m_series{series} {}

  friend struct gauge_metric;

public:
  std::expected<void, int32_t> send_value(double value, k2::SystemTime timestamp = {}) const noexcept {
    if (m_series == k2::INVALID_PLATFORM_DESCRIPTOR) [[unlikely]] {
      return {};
    }
    return k2::metrics_gauge_set(m_series, value, timestamp.since_epoch_ns);
  }
};

struct gauge_metric final {
private:
  k2::descriptor m_metric;

  explicit gauge_metric(k2::descriptor metric) noexcept
      : m_metric{metric} {}

public:
  static gauge_metric metric(std::string_view metric_name, std::initializer_list<std::string_view> label_names) noexcept {
    auto k2_label_names{impl::make_k2_metric_label_names(label_names)};
    auto registered{k2::metrics_register_gauge(metric_name, k2_label_names)};
    if (!registered) [[unlikely]] {
      kphp::log::warning("failed to register `{}` gauge: error -> {}", metric_name, registered.error());
      return gauge_metric{k2::INVALID_PLATFORM_DESCRIPTOR};
    }
    return gauge_metric{*registered};
  }

  gauge_sender series(std::initializer_list<std::pair<std::string_view, std::string_view>> labels) const noexcept {
    if (m_metric == k2::INVALID_PLATFORM_DESCRIPTOR) [[unlikely]] {
      return gauge_sender{k2::INVALID_PLATFORM_DESCRIPTOR};
    }
    auto k2_labels{impl::make_k2_metric_labels(labels)};
    auto bound{k2::metrics_bind(m_metric, k2_labels)};
    if (!bound) [[unlikely]] {
      kphp::log::warning("failed to bind a series of gauge {}: error -> {}", m_metric, bound.error());
      return gauge_sender{k2::INVALID_PLATFORM_DESCRIPTOR};
    }
    return gauge_sender{*bound};
  }
};

struct counter_sender final {
private:
  k2::descriptor m_series;

  explicit counter_sender(k2::descriptor series) noexcept
      : m_series{series} {}

  friend struct counter_metric;

public:
  std::expected<void, int32_t> send_count(uint32_t count, k2::SystemTime timestamp = {}) const noexcept {
    if (m_series == k2::INVALID_PLATFORM_DESCRIPTOR) [[unlikely]] {
      return {};
    }
    return k2::metrics_counter_add(m_series, static_cast<double>(count), timestamp.since_epoch_ns);
  }

  std::expected<void, int32_t> send_increment(k2::SystemTime timestamp = {}) const noexcept {
    return send_count(1, timestamp);
  }
};

struct counter_metric final {
private:
  k2::descriptor m_metric;

  explicit counter_metric(k2::descriptor metric) noexcept
      : m_metric{metric} {}

public:
  static counter_metric metric(std::string_view metric_name, std::initializer_list<std::string_view> label_names) noexcept {
    auto k2_label_names{impl::make_k2_metric_label_names(label_names)};
    auto registered{k2::metrics_register_counter(metric_name, k2_label_names)};
    if (!registered) [[unlikely]] {
      kphp::log::warning("failed to register `{}` counter: error -> {}", metric_name, registered.error());
      return counter_metric{k2::INVALID_PLATFORM_DESCRIPTOR};
    }
    return counter_metric{*registered};
  }

  counter_sender series(std::initializer_list<std::pair<std::string_view, std::string_view>> labels) const noexcept {
    if (m_metric == k2::INVALID_PLATFORM_DESCRIPTOR) [[unlikely]] {
      return counter_sender{k2::INVALID_PLATFORM_DESCRIPTOR};
    }
    auto k2_labels{impl::make_k2_metric_labels(labels)};
    auto bound{k2::metrics_bind(m_metric, k2_labels)};
    if (!bound) [[unlikely]] {
      kphp::log::warning("failed to bind a series of counter {}: error -> {}", m_metric, bound.error());
      return counter_sender{k2::INVALID_PLATFORM_DESCRIPTOR};
    }
    return counter_sender{*bound};
  }
};

struct histogram_sender final {
private:
  k2::descriptor m_series;

  explicit histogram_sender(k2::descriptor series) noexcept
      : m_series{series} {}

  friend struct histogram_metric;

public:
  std::expected<void, int32_t> send_observation(double value, k2::SystemTime timestamp = {}) const noexcept {
    if (m_series == k2::INVALID_PLATFORM_DESCRIPTOR) [[unlikely]] {
      return {};
    }
    return k2::metrics_histogram_observe(m_series, value, timestamp.since_epoch_ns);
  }
};

struct histogram_metric final {
private:
  k2::descriptor m_metric;

  explicit histogram_metric(k2::descriptor metric) noexcept
      : m_metric{metric} {}

public:
  // `buckets` are the explicit upper (`le`) bucket boundaries (finite, strictly ascending);
  // empty registers a histogram without an explicit layout (the monitoring system assigns a default one).
  static histogram_metric metric(std::string_view metric_name, std::span<const double> buckets, std::initializer_list<std::string_view> label_names) noexcept {
    auto k2_label_names{impl::make_k2_metric_label_names(label_names)};
    auto registered{k2::metrics_register_histogram(metric_name, buckets, k2_label_names)};
    if (!registered) [[unlikely]] {
      kphp::log::warning("failed to register `{}` histogram: error -> {}", metric_name, registered.error());
      return histogram_metric{k2::INVALID_PLATFORM_DESCRIPTOR};
    }
    return histogram_metric{*registered};
  }

  histogram_sender series(std::initializer_list<std::pair<std::string_view, std::string_view>> labels) const noexcept {
    if (m_metric == k2::INVALID_PLATFORM_DESCRIPTOR) [[unlikely]] {
      return histogram_sender{k2::INVALID_PLATFORM_DESCRIPTOR};
    }
    auto k2_labels{impl::make_k2_metric_labels(labels)};
    auto bound{k2::metrics_bind(m_metric, k2_labels)};
    if (!bound) [[unlikely]] {
      kphp::log::warning("failed to bind a series of histogram {}: error -> {}", m_metric, bound.error());
      return histogram_sender{k2::INVALID_PLATFORM_DESCRIPTOR};
    }
    return histogram_sender{*bound};
  }
};

} // namespace kphp::diagnostics
