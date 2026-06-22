// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <initializer_list>
#include <memory>
#include <string_view>

#include "common/containers/final_action.h"
#include "common/mixin/movable_only.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/k2-platform/k2-api.h"

namespace kphp::diagnostics {
struct MetricBuilder final : vk::movable_only {
public:
  using bytes_vector = kphp::stl::vector<std::byte, kphp::memory::script_allocator>;

private:
  using string = kphp::stl::string<kphp::memory::script_allocator>;

  string metric_name;
  kphp::stl::vector<std::pair<string, string>, kphp::memory::script_allocator> tags;
  size_t msg_size{0};

  explicit MetricBuilder(std::string_view metric_name) noexcept
      : metric_name{metric_name} {
    this->msg_size += MetricBuilder::string_sizeof(metric_name);
  }

  template<typename T>
  requires std::is_arithmetic_v<T>
  static void store_number(bytes_vector& buf, const T& number) noexcept {
    const auto* src{static_cast<const std::byte*>(static_cast<const void*>(std::addressof(number)))};
    buf.insert(buf.end(), src, src + sizeof(T));
  }

  static void store_string(bytes_vector& buf, const std::string_view& string) noexcept {
    MetricBuilder::store_number(buf, string.size());
    std::transform(string.begin(), string.end(), std::back_inserter(buf), [](char c) { return std::byte(c); });
  }

  static void store_tag(bytes_vector& buf, std::pair<std::string_view, std::string_view> tag) noexcept {
    MetricBuilder::store_string(buf, tag.first);
    MetricBuilder::store_string(buf, tag.second);
  }

  void store_msg(bytes_vector& buf) const noexcept {
    MetricBuilder::store_number(buf, this->msg_size);
    MetricBuilder::store_string(buf, std::string_view{this->metric_name.c_str(), this->metric_name.size()});
    for (const auto& [tag_name, tag_value] : this->tags) {
      MetricBuilder::store_tag(buf, {std::string_view{tag_name.c_str(), tag_name.size()}, std::string_view{tag_value.c_str(), tag_value.size()}});
    }
  }

  auto send_helper(const std::initializer_list<std::pair<std::string_view, std::string_view>>& tags) noexcept {
    size_t tags_size{};
    for (const auto& [tag_name, tag_value] : tags) {
      tags_size += MetricBuilder::string_sizeof(tag_name) + MetricBuilder::string_sizeof(tag_value);
    }
    this->msg_size += tags_size;
    return vk::final_action{[tags_size, this]() { this->msg_size -= tags_size; }};
  }

  static size_t string_sizeof(const std::string_view& string) noexcept {
    return sizeof(size_t) + string.size();
  }

  static uint32_t s_timestamp_now() noexcept {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  }

public:
  static std::expected<void, int32_t> send(bytes_vector& buf, k2::MonitoringSystem ms) noexcept {
    return k2::write_metric(std::span{buf.data(), buf.size()}, ms);
  }

  static MetricBuilder metric(std::string_view metric_name) noexcept {
    return MetricBuilder{metric_name};
  }

  MetricBuilder& tag(std::string_view tag_name, std::string_view tag_value) noexcept {
    this->tags.emplace_back(tag_name, tag_value);
    this->msg_size += MetricBuilder::string_sizeof(tag_name) + MetricBuilder::string_sizeof(tag_value);
    return *this;
  }

  bytes_vector build_value(double value, std::optional<uint32_t> timestamp = std::nullopt) const noexcept {
    bytes_vector buf{};
    buf.reserve(sizeof(uint32_t) + sizeof(uint8_t) + sizeof(double) + sizeof(size_t) +
                this->msg_size); // timestamp_u32 + value_kind_u8 + value_f64 + msg_size_usize + msg_len

    uint32_t s_timestamp{timestamp.value_or(MetricBuilder::s_timestamp_now())};

    MetricBuilder::store_number(buf, s_timestamp);
    MetricBuilder::store_number(buf, static_cast<uint8_t>(k2::MetricValueKind::VALUE));
    MetricBuilder::store_number(buf, value);
    this->store_msg(buf);
    return buf;
  }

  bytes_vector build_values_array(std::initializer_list<double> values, std::optional<uint32_t> timestamp = std::nullopt) const noexcept {
    bytes_vector buf{};
    buf.reserve(sizeof(uint32_t) + sizeof(uint8_t) + sizeof(size_t) + sizeof(double) * values.size() + sizeof(size_t) +
                this->msg_size); // timestamp_u32 + value_kind_u8 + array_len_usize + value_f64*array_len + msg_size_usize + msg_len

    uint32_t s_timestamp{timestamp.value_or(MetricBuilder::s_timestamp_now())};

    MetricBuilder::store_number(buf, s_timestamp);
    MetricBuilder::store_number(buf, static_cast<uint8_t>(k2::MetricValueKind::VALUES_ARRAY));
    MetricBuilder::store_number(buf, values.size());
    for (const auto& value : values) {
      MetricBuilder::store_number(buf, value);
    }
    this->store_msg(buf);
    return buf;
  }

  bytes_vector build_count(double count, std::optional<uint32_t> timestamp = std::nullopt) const noexcept {
    bytes_vector buf{};
    buf.reserve(sizeof(uint32_t) + sizeof(uint8_t) + sizeof(double) + sizeof(size_t) +
                this->msg_size); // timestamp_u32 + value_kind_u8 + count_f64 + msg_size_usize + msg_len

    uint32_t s_timestamp{timestamp.value_or(MetricBuilder::s_timestamp_now())};

    MetricBuilder::store_number(buf, s_timestamp);
    MetricBuilder::store_number(buf, static_cast<uint8_t>(k2::MetricValueKind::COUNT));
    MetricBuilder::store_number(buf, count);
    this->store_msg(buf);
    return buf;
  }

  bytes_vector build_increment(std::optional<uint32_t> timestamp = std::nullopt) const noexcept {
    bytes_vector buf{};
    buf.reserve(sizeof(uint32_t) + sizeof(uint8_t) + sizeof(size_t) + this->msg_size); // timestamp_u32 + value_kind_u8 + msg_size_usize + msg_len

    uint32_t s_timestamp{timestamp.value_or(MetricBuilder::s_timestamp_now())};

    MetricBuilder::store_number(buf, s_timestamp);
    MetricBuilder::store_number(buf, static_cast<uint8_t>(k2::MetricValueKind::INC));
    this->store_msg(buf);
    return buf;
  }

  std::expected<void, int32_t> send_value(double value, k2::MonitoringSystem ms, std::initializer_list<std::pair<std::string_view, std::string_view>> tags = {},
                                          std::optional<uint32_t> timestamp = std::nullopt) noexcept {
    auto final_action{send_helper(tags)};

    auto buf{this->build_value(value, timestamp)};
    for (const auto& tag : tags) {
      MetricBuilder::store_tag(buf, tag);
    }

    return MetricBuilder::send(buf, ms);
  }

  std::expected<void, int32_t> send_values_array(std::initializer_list<double> values, k2::MonitoringSystem ms,
                                                 std::initializer_list<std::pair<std::string_view, std::string_view>> tags = {},
                                                 std::optional<uint32_t> timestamp = std::nullopt) noexcept {
    auto final_action{send_helper(tags)};

    auto buf{this->build_values_array(values, timestamp)};
    for (const auto& tag : tags) {
      MetricBuilder::store_tag(buf, tag);
    }

    return MetricBuilder::send(buf, ms);
  }

  std::expected<void, int32_t> send_count(double count, k2::MonitoringSystem ms, std::initializer_list<std::pair<std::string_view, std::string_view>> tags = {},
                                          std::optional<uint32_t> timestamp = std::nullopt) noexcept {
    auto final_action{send_helper(tags)};

    auto buf{this->build_count(count, timestamp)};
    for (const auto& tag : tags) {
      MetricBuilder::store_tag(buf, tag);
    }

    return MetricBuilder::send(buf, ms);
  }

  std::expected<void, int32_t> send_increment(k2::MonitoringSystem ms, std::initializer_list<std::pair<std::string_view, std::string_view>> tags = {},
                                              std::optional<uint32_t> timestamp = std::nullopt) noexcept {
    auto final_action{send_helper(tags)};

    auto buf{this->build_increment(timestamp)};
    for (const auto& tag : tags) {
      MetricBuilder::store_tag(buf, tag);
    }

    return MetricBuilder::send(buf, ms);
  }
};
} // namespace kphp::diagnostics
