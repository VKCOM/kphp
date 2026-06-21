// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

#include "common/mixin/movable_only.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/k2-platform/k2-api.h"

struct MetricsBuffer final : vk::movable_only {
private:
  using bytes_vector = kphp::stl::vector<uint8_t, kphp::memory::script_allocator>;

  std::string metric_name;
  kphp::stl::unordered_map<std::string, std::string, kphp::memory::script_allocator> tags;
  size_t msg_size{0};

  explicit MetricsBuffer(std::string_view metric_name) noexcept
      : metric_name{metric_name} {
    this->msg_size += MetricsBuffer::string_sizeof(metric_name);
  }

  template<typename T>
  requires std::is_arithmetic_v<T>
  static void store_number(bytes_vector& buf, const T& number) noexcept {
    const auto* src = static_cast<const uint8_t*>(static_cast<const void*>(&number));
    buf.insert(buf.end(), src, src + sizeof(T));
  }

  static void store_string(bytes_vector& buf, const std::string_view& string) noexcept {
    MetricsBuffer::store_number(buf, string.size());
    buf.insert(buf.end(), string.begin(), string.end());
  }

  void store_msg(bytes_vector& buf) const noexcept {
    MetricsBuffer::store_number(buf, this->msg_size);
    MetricsBuffer::store_string(buf, std::string_view{this->metric_name.c_str(), this->metric_name.size()});
    for (const auto& [tag_name, tag_value] : this->tags) {
      MetricsBuffer::store_string(buf, std::string_view{tag_name.c_str(), tag_name.size()});
      MetricsBuffer::store_string(buf, std::string_view{tag_value.c_str(), tag_value.size()});
    }
  }

  static size_t string_sizeof(const std::string_view& string) noexcept {
    return sizeof(size_t) + string.size();
  }

  static uint32_t s_timestamp_now() noexcept {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  }

public:
  static MetricsBuffer metric(std::string_view metric_name) noexcept {
    return MetricsBuffer{metric_name};
  }

  MetricsBuffer& tag(std::string_view tag_name, std::string_view tag_value) noexcept {
    this->tags[std::string{tag_name}] = std::string{tag_value};
    this->msg_size += MetricsBuffer::string_sizeof(tag_name) + MetricsBuffer::string_sizeof(tag_value);
    return *this;
  }

  bytes_vector build_value(double value, std::optional<uint32_t> timestamp = std::nullopt) const noexcept {
    bytes_vector buf{};
    buf.reserve(sizeof(uint32_t) + sizeof(uint32_t) + sizeof(double) + sizeof(size_t) +
                this->msg_size); // timestamp_u32 + value_mask_u32 + value_f64 + msg_size_usize + msg_len

    uint32_t s_timestamp{timestamp.value_or(MetricsBuffer::s_timestamp_now())};

    MetricsBuffer::store_number(buf, s_timestamp);
    MetricsBuffer::store_number(buf, static_cast<uint32_t>(k2::MetricValueMask::VALUE_MASK));
    MetricsBuffer::store_number(buf, value);
    this->store_msg(buf);
    return buf;
  }

  bytes_vector build_values_array(const kphp::stl::vector<double, kphp::memory::script_allocator>& values,
                                  std::optional<uint32_t> timestamp = std::nullopt) const noexcept {
    bytes_vector buf{};
    buf.reserve(sizeof(uint32_t) + sizeof(uint32_t) + sizeof(size_t) + sizeof(double) * values.size() + sizeof(size_t) +
                this->msg_size); // timestamp_u32 + value_mask_u32 + array_len_usize + value_f64*array_len + msg_size_usize + msg_len

    uint32_t s_timestamp{timestamp.value_or(MetricsBuffer::s_timestamp_now())};

    MetricsBuffer::store_number(buf, s_timestamp);
    MetricsBuffer::store_number(buf, static_cast<uint32_t>(k2::MetricValueMask::VALUES_ARRAY_MASK));
    MetricsBuffer::store_number(buf, values.size());
    for (const auto& value : values) {
      MetricsBuffer::store_number(buf, value);
    }
    this->store_msg(buf);
    return buf;
  }

  bytes_vector build_count(double count, std::optional<uint32_t> timestamp = std::nullopt) const noexcept {
    bytes_vector buf{};
    buf.reserve(sizeof(uint32_t) + sizeof(uint32_t) + sizeof(double) + sizeof(size_t) +
                this->msg_size); // timestamp_u32 + value_mask_u32 + count_f64 + msg_size_usize + msg_len

    uint32_t s_timestamp{timestamp.value_or(MetricsBuffer::s_timestamp_now())};

    MetricsBuffer::store_number(buf, s_timestamp);
    MetricsBuffer::store_number(buf, static_cast<uint32_t>(k2::MetricValueMask::COUNT_MASK));
    MetricsBuffer::store_number(buf, count);
    this->store_msg(buf);
    return buf;
  }

  bytes_vector build_increment(std::optional<uint32_t> timestamp = std::nullopt) const noexcept {
    bytes_vector buf{};
    buf.reserve(sizeof(uint32_t) + sizeof(uint32_t) + sizeof(size_t) + this->msg_size); // timestamp_u32 + value_mask_u32 + msg_size_usize + msg_len

    uint32_t s_timestamp{timestamp.value_or(MetricsBuffer::s_timestamp_now())};

    MetricsBuffer::store_number(buf, s_timestamp);
    MetricsBuffer::store_number(buf, static_cast<uint32_t>(k2::MetricValueMask::INC_MASK));
    this->store_msg(buf);
    return buf;
  }
};
