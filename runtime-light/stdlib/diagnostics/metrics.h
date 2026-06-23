// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <ranges>
#include <span>
#include <string_view>
#include <utility>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/k2-platform/k2-api.h"

namespace kphp::diagnostics {

namespace details {
inline size_t string_sizeof(const std::string_view& string) noexcept {
  return sizeof(size_t) + string.size();
}
} // namespace details

template<typename T>
concept tag_range = std::ranges::range<T> && requires(const std::ranges::range_value_t<T>& tag) {
  std::string_view{tag.first};
  std::string_view{tag.second};
};

struct metric final {
private:
  using bytes_vector = kphp::stl::vector<std::byte, kphp::memory::script_allocator>;

  const bytes_vector buffer;
  k2::MonitoringSystem ms;

  metric(bytes_vector buffer, k2::MonitoringSystem ms) noexcept
      : buffer{std::move(buffer)},
        ms{ms} {}

  template<typename T>
  requires std::is_arithmetic_v<T>
  static void store_number(bytes_vector& buf, const T& number) noexcept {
    const auto* src{static_cast<const std::byte*>(static_cast<const void*>(std::addressof(number)))};
    buf.insert(buf.end(), src, src + sizeof(T));
  }

  static void store_string(bytes_vector& buf, const std::string_view& string) noexcept {
    metric::store_number(buf, string.size());
    buf.append_range(std::as_bytes(std::span{string.data(), string.size()}));
  }

  static void store_tag(bytes_vector& buf, std::string_view tag_name, std::string_view tag_value) noexcept {
    metric::store_string(buf, tag_name);
    metric::store_string(buf, tag_value);
  }

  template<tag_range TagRange>
  static void store_msg(bytes_vector& buf, std::string_view metric_name, size_t msg_len, TagRange&& tags) noexcept {
    metric::store_number(buf, msg_len);
    metric::store_string(buf, metric_name);
    for (const auto& [tag_name, tag_value] : std::forward<TagRange>(tags)) {
      metric::store_tag(buf, std::string_view{tag_name}, std::string_view{tag_value});
    }
  }

  static uint64_t ns_timestamp_now() noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  }

  template<tag_range TagRange>
  static size_t calc_msg_len(std::string_view metric_name, TagRange&& tags) noexcept {
    size_t result{kphp::diagnostics::details::string_sizeof(metric_name)};
    for (const auto& [tag_name, tag_value] : std::forward<TagRange>(tags)) {
      result += kphp::diagnostics::details::string_sizeof(std::string_view{tag_name}) + kphp::diagnostics::details::string_sizeof(std::string_view{tag_value});
    }
    return result;
  }

public:
  template<tag_range TagRange>
  static metric from_value(k2::MonitoringSystem ms, std::string_view metric_name, TagRange&& tags, double value,
                           std::optional<uint64_t> timestamp = std::nullopt) noexcept {
    size_t msg_len{metric::calc_msg_len(metric_name, std::forward<TagRange>(tags))};

    bytes_vector buf{};
    buf.reserve(sizeof(uint64_t) + sizeof(uint8_t) + sizeof(double) + sizeof(size_t) + msg_len); // timestamp_u64 + value_kind_u8 + value_f64 + msg_len + msg

    uint64_t ns_timestamp{timestamp.value_or(metric::ns_timestamp_now())};

    metric::store_number(buf, ns_timestamp);
    metric::store_number(buf, static_cast<uint8_t>(k2::MetricValueKind::VALUE));
    metric::store_number(buf, value);
    metric::store_msg(buf, metric_name, msg_len, std::forward<TagRange>(tags));
    return metric{std::move(buf), ms};
  }

  template<tag_range TagRange>
  static metric from_values_array(k2::MonitoringSystem ms, std::string_view metric_name, TagRange&& tags, std::span<const double> values,
                                  std::optional<uint64_t> timestamp = std::nullopt) noexcept {
    size_t msg_len{metric::calc_msg_len(metric_name, std::forward<TagRange>(tags))};

    bytes_vector buf{};
    buf.reserve(sizeof(uint64_t) + sizeof(uint8_t) + sizeof(size_t) + sizeof(double) * values.size() + sizeof(size_t) +
                msg_len); // timestamp_u64 + value_kind_u8 + array_len_usize + value_f64*array_len + msg_len + msg

    uint64_t ns_timestamp{timestamp.value_or(metric::ns_timestamp_now())};

    metric::store_number(buf, ns_timestamp);
    metric::store_number(buf, static_cast<uint8_t>(k2::MetricValueKind::VALUES_ARRAY));
    metric::store_number(buf, values.size());
    for (const auto& value : values) {
      metric::store_number(buf, value);
    }
    metric::store_msg(buf, metric_name, msg_len, std::forward<TagRange>(tags));
    return metric{std::move(buf), ms};
  }

  template<tag_range TagRange>
  static metric from_count(k2::MonitoringSystem ms, std::string_view metric_name, TagRange&& tags, uint32_t count,
                           std::optional<uint64_t> timestamp = std::nullopt) noexcept {
    size_t msg_len{metric::calc_msg_len(metric_name, std::forward<TagRange>(tags))};

    bytes_vector buf{};
    buf.reserve(sizeof(uint64_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(size_t) + msg_len); // timestamp_u64 + value_kind_u8 + count_u32 + msg_len + msg

    uint64_t ns_timestamp{timestamp.value_or(metric::ns_timestamp_now())};

    metric::store_number(buf, ns_timestamp);
    metric::store_number(buf, static_cast<uint8_t>(k2::MetricValueKind::COUNT));
    metric::store_number(buf, count);
    metric::store_msg(buf, metric_name, msg_len, std::forward<TagRange>(tags));

    return metric{std::move(buf), ms};
  }

  template<tag_range TagRange>
  static metric from_increment(k2::MonitoringSystem ms, std::string_view metric_name, TagRange&& tags,
                               std::optional<uint64_t> timestamp = std::nullopt) noexcept {
    size_t msg_len{metric::calc_msg_len(metric_name, std::forward<TagRange>(tags))};

    bytes_vector buf{};
    buf.reserve(sizeof(uint64_t) + sizeof(uint8_t) + sizeof(size_t) + msg_len); // timestamp_u64 + value_kind_u8 + msg_len + msg

    uint64_t ns_timestamp{timestamp.value_or(metric::ns_timestamp_now())};

    metric::store_number(buf, ns_timestamp);
    metric::store_number(buf, static_cast<uint8_t>(k2::MetricValueKind::INC));
    metric::store_msg(buf, metric_name, msg_len, std::forward<TagRange>(tags));
    return metric{std::move(buf), ms};
  }

  std::expected<void, int32_t> send() const noexcept {
    return k2::write_metric(this->buffer, this->ms);
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

  kphp::diagnostics::metric build_value(double value, std::optional<uint64_t> timestamp = std::nullopt) const noexcept {
    return metric::from_value(this->ms, this->metric_name, this->tags, value, timestamp);
  }

  kphp::diagnostics::metric build_values_array(std::span<const double> values, std::optional<uint64_t> timestamp = std::nullopt) const noexcept {
    return metric::from_values_array(this->ms, this->metric_name, this->tags, values, timestamp);
  }

  kphp::diagnostics::metric build_count(uint32_t count, std::optional<uint64_t> timestamp = std::nullopt) const noexcept {
    return metric::from_count(this->ms, this->metric_name, this->tags, count, timestamp);
  }

  kphp::diagnostics::metric build_increment(std::optional<uint64_t> timestamp = std::nullopt) const noexcept {
    return metric::from_increment(this->ms, this->metric_name, this->tags, timestamp);
  }
};
} // namespace kphp::diagnostics
