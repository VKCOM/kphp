// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-types.h"

namespace kphp::diagnostics {
template<typename T>
concept tag_range = std::ranges::range<T> && std::is_constructible_v<std::pair<std::string_view, std::string_view>, std::ranges::range_value_t<T>>;

struct metric final {
private:
  tl::storer tls;
  k2::MonitoringSystem ms;

  explicit metric(k2::MonitoringSystem ms) noexcept
      : tls{0},
        ms{ms} {}

  metric(tl::storer&& tls, k2::MonitoringSystem ms) noexcept
      : tls{std::move(tls)},
        ms{ms} {}

  static uint64_t ns_timestamp_now() noexcept {
    k2::SystemTime st{};
    k2::system_time(std::addressof(st));
    return st.since_epoch_ns;
  }

  std::expected<void, int32_t> send() const noexcept {
    return k2::write_metrics(this->tls.view(), this->ms);
  }

  // clears buffer and returns it with preserved capacity for reuse by metric::with_buffer()
  std::expected<tl::storer, int32_t> send() && noexcept {
    return k2::write_metrics(this->tls.view(), this->ms).transform([this]() noexcept {
      this->tls.clear();
      return std::move(this->tls);
    });
  }

  template<typename Self, tag_range TagRange, typename ValueRange = void>
  decltype(auto) build_and_send(this Self&& self, std::string_view metric_name, TagRange&& tags, tl::metricValueFormat<ValueRange> value,
                                std::optional<uint64_t> timestamp) noexcept {
    self.tls.clear();

    uint64_t ns_timestamp{timestamp.value_or(metric::ns_timestamp_now())};
    tl::metric serialized{.timestamp = tl::u64{ns_timestamp},
                          .value = value,
                          .metric_name = tl::string{metric_name},
                          .tags = std::forward<TagRange>(tags) | std::views::transform([](const auto& elem) noexcept -> tl::pair<tl::string, tl::string> {
                                    std::pair<std::string_view, std::string_view> sv_pair{elem};
                                    return tl::pair{std::pair{tl::string{sv_pair.first}, tl::string{sv_pair.second}}};
                                  })};

    self.tls.reserve(serialized.footprint());
    serialized.store(self.tls);

    return std::forward<Self>(self).send();
  }

public:
  static metric empty(k2::MonitoringSystem ms) noexcept {
    return metric{ms};
  }

  static metric with_buffer(tl::storer&& tls, k2::MonitoringSystem ms) noexcept {
    return metric{std::move(tls), ms};
  }

  template<typename Self, tag_range TagRange>
  decltype(auto) send_value(this Self&& self, std::string_view metric_name, TagRange&& tags, double value,
                            std::optional<uint64_t> timestamp = std::nullopt) noexcept {
    return std::forward<Self>(self).build_and_send(metric_name, std::forward<TagRange>(tags), tl::metricValueFormat<>{tl::MetricValue{tl::f64{value}}},
                                                   timestamp);
  }

  template<typename Self, tag_range TagRange, std::ranges::range ValueRange>
  requires std::same_as<std::remove_cvref_t<std::ranges::range_value_t<ValueRange>>, tl::f64>
  decltype(auto) send_values_array(this Self&& self, std::string_view metric_name, TagRange&& tags, ValueRange&& values,
                                   std::optional<uint64_t> timestamp = std::nullopt) noexcept {
    using range_t = std::remove_cvref_t<ValueRange>;
    return std::forward<Self>(self).build_and_send(metric_name, std::forward<TagRange>(tags),
                                                   tl::metricValueFormat<range_t>{tl::MetricValuesArray<range_t>{.values = std::forward<ValueRange>(values)}},
                                                   timestamp);
  }

  template<typename Self, tag_range TagRange>
  decltype(auto) send_count(this Self&& self, std::string_view metric_name, TagRange&& tags, uint32_t count,
                            std::optional<uint64_t> timestamp = std::nullopt) noexcept {
    return std::forward<Self>(self).build_and_send(metric_name, std::forward<TagRange>(tags), tl::metricValueFormat<>{tl::MetricCount{tl::u32{count}}},
                                                   timestamp);
  }

  template<typename Self, tag_range TagRange>
  decltype(auto) send_increment(this Self&& self, std::string_view metric_name, TagRange&& tags, std::optional<uint64_t> timestamp = std::nullopt) noexcept {
    return std::forward<Self>(self).build_and_send(metric_name, std::forward<TagRange>(tags), tl::metricValueFormat<>{tl::MetricInc{}}, timestamp);
  }
};

// ---------------------------------------------------------------------------------------------------------

struct metric_builder final {
private:
  kphp::stl::string<kphp::memory::script_allocator> metric_name;
  kphp::stl::vector<std::pair<kphp::stl::string<kphp::memory::script_allocator>, kphp::stl::string<kphp::memory::script_allocator>>,
                    kphp::memory::script_allocator>
      tags;
  k2::MonitoringSystem ms;

  metric_builder(std::string_view metric_name, k2::MonitoringSystem ms) noexcept
      : metric_name{metric_name},
        ms{ms} {}

  static auto discard_buffer() noexcept {
    return [](const auto&) noexcept {};
  }

public:
  static metric_builder metric(std::string_view metric_name, k2::MonitoringSystem ms) noexcept {
    return metric_builder{metric_name, ms};
  }

  metric_builder& tag(std::string_view tag_name, std::string_view tag_value) noexcept {
    this->tags.emplace_back(tag_name, tag_value);
    return *this;
  }

  auto send_value(double value, std::optional<uint64_t> timestamp = std::nullopt) const noexcept {
    return metric::empty(this->ms).send_value(this->metric_name, this->tags, value, timestamp).transform(discard_buffer());
  }

  auto send_values_array(std::span<const double> values, std::optional<uint64_t> timestamp = std::nullopt) const noexcept {
    return metric::empty(this->ms)
        .send_values_array(this->metric_name, this->tags, values | std::views::transform([](const double& value) noexcept { return tl::f64{value}; }),
                           timestamp)
        .transform(discard_buffer());
  }

  auto send_count(uint32_t count, std::optional<uint64_t> timestamp = std::nullopt) const noexcept {
    return metric::empty(this->ms).send_count(this->metric_name, this->tags, count, timestamp).transform(discard_buffer());
  }

  auto send_increment(std::optional<uint64_t> timestamp = std::nullopt) const noexcept {
    return metric::empty(this->ms).send_increment(this->metric_name, this->tags, timestamp).transform(discard_buffer());
  }
};
} // namespace kphp::diagnostics
