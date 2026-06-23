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
#include <type_traits>
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
concept tag_range = std::ranges::range<T> && std::is_constructible_v<std::pair<std::string_view, std::string_view>, std::ranges::range_value_t<T>>;

struct metric final {
public:
  using serialize_buffer = kphp::stl::vector<std::byte, kphp::memory::script_allocator>;

private:
  serialize_buffer buffer;
  k2::MonitoringSystem ms;

  explicit metric(k2::MonitoringSystem ms) noexcept
      : buffer{serialize_buffer{}},
        ms{ms} {}

  metric(serialize_buffer&& buffer, k2::MonitoringSystem ms) noexcept
      : buffer{std::move(buffer)},
        ms{ms} {}

  template<typename T>
  requires std::is_arithmetic_v<T>
  static void store_number(serialize_buffer& buf, const T& number) noexcept {
    const auto* src{static_cast<const std::byte*>(static_cast<const void*>(std::addressof(number)))};
    buf.insert(buf.end(), src, src + sizeof(T));
  }

  static void store_string(serialize_buffer& buf, const std::string_view& string) noexcept {
    metric::store_number(buf, string.size());
    buf.append_range(std::as_bytes(std::span{string.data(), string.size()}));
  }

  static void store_tag(serialize_buffer& buf, std::string_view tag_name, std::string_view tag_value) noexcept {
    metric::store_string(buf, tag_name);
    metric::store_string(buf, tag_value);
  }

  template<tag_range TagRange>
  static void store_msg(serialize_buffer& buf, std::string_view metric_name, size_t msg_len, TagRange&& tags) noexcept {
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
  static metric empty(k2::MonitoringSystem ms) noexcept {
    return metric{ms};
  }

  static metric with_buffer(serialize_buffer&& buffer, k2::MonitoringSystem ms) noexcept {
    return metric{std::move(buffer), ms};
  }

  template<typename Self, tag_range TagRange>
  decltype(auto) build_value(this Self&& self, std::string_view metric_name, TagRange&& tags, double value,
                             std::optional<uint64_t> timestamp = std::nullopt) noexcept {
    self.buffer.clear();
    size_t msg_len{metric::calc_msg_len(metric_name, std::forward<TagRange>(tags))};
    self.buffer.reserve(sizeof(uint64_t) + sizeof(uint8_t) + sizeof(double) + sizeof(size_t) +
                        msg_len); // timestamp_u64 + value_kind_u8 + value_f64 + msg_len + msg

    uint64_t ns_timestamp{timestamp.value_or(metric::ns_timestamp_now())};

    metric::store_number(self.buffer, ns_timestamp);
    metric::store_number(self.buffer, static_cast<uint8_t>(k2::MetricValueKind::VALUE));
    metric::store_number(self.buffer, value);
    metric::store_msg(self.buffer, metric_name, msg_len, std::forward<TagRange>(tags));
    return std::forward<Self>(self);
  }

  template<typename Self, tag_range TagRange>
  decltype(auto) build_values_array(this Self&& self, std::string_view metric_name, TagRange&& tags, std::span<const double> values,
                                    std::optional<uint64_t> timestamp = std::nullopt) noexcept {
    self.buffer.clear();
    size_t msg_len{metric::calc_msg_len(metric_name, std::forward<TagRange>(tags))};
    self.buffer.reserve(sizeof(uint64_t) + sizeof(uint8_t) + sizeof(size_t) + sizeof(double) * values.size() + sizeof(size_t) +
                        msg_len); // timestamp_u64 + value_kind_u8 + array_len_usize + value_f64*array_len + msg_len + msg

    uint64_t ns_timestamp{timestamp.value_or(metric::ns_timestamp_now())};

    metric::store_number(self.buffer, ns_timestamp);
    metric::store_number(self.buffer, static_cast<uint8_t>(k2::MetricValueKind::VALUES_ARRAY));
    metric::store_number(self.buffer, values.size());
    for (const auto& value : values) {
      metric::store_number(self.buffer, value);
    }
    metric::store_msg(self.buffer, metric_name, msg_len, std::forward<TagRange>(tags));
    return std::forward<Self>(self);
  }

  template<typename Self, tag_range TagRange>
  decltype(auto) build_count(this Self&& self, std::string_view metric_name, TagRange&& tags, uint32_t count,
                             std::optional<uint64_t> timestamp = std::nullopt) noexcept {
    self.buffer.clear();
    size_t msg_len{metric::calc_msg_len(metric_name, std::forward<TagRange>(tags))};
    self.buffer.reserve(sizeof(uint64_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(size_t) +
                        msg_len); // timestamp_u64 + value_kind_u8 + count_u32 + msg_len + msg

    uint64_t ns_timestamp{timestamp.value_or(metric::ns_timestamp_now())};

    metric::store_number(self.buffer, ns_timestamp);
    metric::store_number(self.buffer, static_cast<uint8_t>(k2::MetricValueKind::COUNT));
    metric::store_number(self.buffer, count);
    metric::store_msg(self.buffer, metric_name, msg_len, std::forward<TagRange>(tags));

    return std::forward<Self>(self);
  }

  template<typename Self, tag_range TagRange>
  decltype(auto) build_increment(this Self&& self, std::string_view metric_name, TagRange&& tags, std::optional<uint64_t> timestamp = std::nullopt) noexcept {
    self.buffer.clear();
    size_t msg_len{metric::calc_msg_len(metric_name, std::forward<TagRange>(tags))};
    self.buffer.reserve(sizeof(uint64_t) + sizeof(uint8_t) + sizeof(size_t) + msg_len); // timestamp_u64 + value_kind_u8 + msg_len + msg

    uint64_t ns_timestamp{timestamp.value_or(metric::ns_timestamp_now())};

    metric::store_number(self.buffer, ns_timestamp);
    metric::store_number(self.buffer, static_cast<uint8_t>(k2::MetricValueKind::INC));
    metric::store_msg(self.buffer, metric_name, msg_len, std::forward<TagRange>(tags));
    return std::forward<Self>(self);
  }

  std::expected<void, int32_t> send() const noexcept {
    return k2::write_metric(this->buffer, this->ms);
  }

  std::expected<serialize_buffer, int32_t> send() && noexcept {
    return k2::write_metric(this->buffer, this->ms).transform([this]() noexcept {
      this->buffer.clear();
      return std::move(this->buffer);
    });
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
    return metric::empty(this->ms).build_value(this->metric_name, this->tags, value, timestamp);
  }

  kphp::diagnostics::metric build_values_array(std::span<const double> values, std::optional<uint64_t> timestamp = std::nullopt) const noexcept {
    return metric::empty(this->ms).build_values_array(this->metric_name, this->tags, values, timestamp);
  }

  kphp::diagnostics::metric build_count(uint32_t count, std::optional<uint64_t> timestamp = std::nullopt) const noexcept {
    return metric::empty(this->ms).build_count(this->metric_name, this->tags, count, timestamp);
  }

  kphp::diagnostics::metric build_increment(std::optional<uint64_t> timestamp = std::nullopt) const noexcept {
    return metric::empty(this->ms).build_increment(this->metric_name, this->tags, timestamp);
  }
};
} // namespace kphp::diagnostics
