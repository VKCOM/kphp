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

namespace tl {

class MetricValue final {
  static constexpr uint32_t MAGIC = 0xcb5eb87U;

public:
  tl::f64 value;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = MetricValue::MAGIC}.store(tls);
    value.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = MetricValue::MAGIC}.footprint() + value.footprint();
  }
};

class MetricValuesArray final {
  static constexpr uint32_t MAGIC = 0xd4a59582U;

public:
  std::span<const double> values;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = MetricValuesArray::MAGIC}.store(tls);
    tl::u32{static_cast<uint32_t>(this->values.size())}.store(tls);
    std::ranges::for_each(this->values, [&tls](const double& elem) noexcept { tl::f64{elem}.store(tls); });
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = MetricValuesArray::MAGIC}.footprint() +
           std::ranges::fold_left(this->values, tl::u32{static_cast<uint32_t>(this->values.size())}.footprint(),
                                  [](size_t acc, const double& elem) noexcept { return acc + tl::f64{elem}.footprint(); });
  }
};

class MetricCount final {
  static constexpr uint32_t MAGIC = 0x941bf7d1U;

public:
  tl::u32 count;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = MetricCount::MAGIC}.store(tls);
    count.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = MetricCount::MAGIC}.footprint() + count.footprint();
  }
};

class MetricInc final {
  static constexpr uint32_t MAGIC = 0x23e305abU;

public:
  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = MetricInc::MAGIC}.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = MetricInc::MAGIC}.footprint();
  }
};

struct anyMetricValue final {
  std::variant<MetricValue, MetricValuesArray, MetricCount, MetricInc> value;

  void store(tl::storer& tls) const noexcept {
    std::visit([&tls](const auto& v) noexcept { v.store(tls); }, value);
  }

  constexpr size_t footprint() const noexcept {
    return std::visit([](const auto& v) noexcept { return v.footprint(); }, value);
  }
};

template<std::ranges::range TagRange, typename ValueRange = void>
requires std::same_as<std::remove_cvref_t<std::ranges::range_value_t<TagRange>>, tl::pair<tl::string, tl::string>>
struct metric final {
  tl::u64 timestamp;
  tl::anyMetricValue value;
  tl::string metric_name;
  TagRange tags;

  void store(tl::storer& tls) const noexcept {
    timestamp.store(tls);
    value.store(tls);
    metric_name.store(tls);

    tl::u32{.value = static_cast<uint32_t>(std::ranges::distance(tags))}.store(tls);
    std::ranges::for_each(tags, [&tls](const auto& elem) noexcept { elem.store(tls); });
  }

  constexpr size_t footprint() const noexcept {
    return timestamp.footprint() + value.footprint() + metric_name.footprint() +
           std::ranges::fold_left(tags, tl::u32{.value = static_cast<uint32_t>(std::ranges::distance(tags))}.footprint(),
                                  [](size_t acc, const auto& elem) noexcept { return acc + elem.footprint(); });
  }
};
} // namespace tl

// ---------------------------------------------------------------------------------------------------------

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

  template<typename Self, tag_range TagRange>
  decltype(auto) build_and_send(this Self&& self, std::string_view metric_name, TagRange&& tags, tl::anyMetricValue value,
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
    return std::forward<Self>(self).build_and_send(metric_name, std::forward<TagRange>(tags), tl::anyMetricValue{tl::MetricValue{tl::f64{value}}}, timestamp);
  }

  template<typename Self, tag_range TagRange>
  decltype(auto) send_values_array(this Self&& self, std::string_view metric_name, TagRange&& tags, std::span<const double> values,
                                   std::optional<uint64_t> timestamp = std::nullopt) noexcept {
    return std::forward<Self>(self).build_and_send(metric_name, std::forward<TagRange>(tags), tl::anyMetricValue{tl::MetricValuesArray{values}}, timestamp);
  }

  template<typename Self, tag_range TagRange>
  decltype(auto) send_count(this Self&& self, std::string_view metric_name, TagRange&& tags, uint32_t count,
                            std::optional<uint64_t> timestamp = std::nullopt) noexcept {
    return std::forward<Self>(self).build_and_send(metric_name, std::forward<TagRange>(tags), tl::anyMetricValue{tl::MetricCount{tl::u32{count}}}, timestamp);
  }

  template<typename Self, tag_range TagRange>
  decltype(auto) send_increment(this Self&& self, std::string_view metric_name, TagRange&& tags, std::optional<uint64_t> timestamp = std::nullopt) noexcept {
    return std::forward<Self>(self).build_and_send(metric_name, std::forward<TagRange>(tags), tl::anyMetricValue{tl::MetricInc{}}, timestamp);
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

public:
  static metric_builder metric(std::string_view metric_name, k2::MonitoringSystem ms) noexcept {
    return metric_builder{metric_name, ms};
  }

  metric_builder& tag(std::string_view tag_name, std::string_view tag_value) noexcept {
    this->tags.emplace_back(tag_name, tag_value);
    return *this;
  }

  auto send_value(double value, std::optional<uint64_t> timestamp = std::nullopt) const noexcept {
    return metric::empty(this->ms).send_value(this->metric_name, this->tags, value, timestamp);
  }

  auto send_values_array(std::span<const double> values, std::optional<uint64_t> timestamp = std::nullopt) const noexcept {
    return metric::empty(this->ms).send_values_array(this->metric_name, this->tags, values, timestamp);
  }

  auto send_count(uint32_t count, std::optional<uint64_t> timestamp = std::nullopt) const noexcept {
    return metric::empty(this->ms).send_count(this->metric_name, this->tags, count, timestamp);
  }

  auto send_increment(std::optional<uint64_t> timestamp = std::nullopt) const noexcept {
    return metric::empty(this->ms).send_increment(this->metric_name, this->tags, timestamp);
  }
};
} // namespace kphp::diagnostics
