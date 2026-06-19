// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/k2-platform/k2-api.h"

struct MetricBuffer {
private:
  using buf_vector = kphp::stl::vector<uint8_t, kphp::memory::script_allocator>;
  using hasher_type = decltype([](const string& s) noexcept { return static_cast<size_t>(s.hash()); });

  string metric_name;
  kphp::stl::unordered_map<string, string, kphp::memory::script_allocator, hasher_type> tags;
  size_t msg_size{0};

  MetricBuffer(std::string_view metric_name) noexcept
      : metric_name{metric_name.data(), static_cast<string::size_type>(metric_name.length())},
        tags{} {
    this->msg_size += MetricBuffer::string_sizeof(metric_name);
  }

  template<typename T>
  requires std::is_arithmetic_v<T>
  static void store_number(buf_vector& buf, const T& number) noexcept {
    const auto* src = static_cast<const uint8_t*>(static_cast<const void*>(&number));
    buf.insert(buf.end(), src, src + sizeof(T));
  }

  static void store_string(buf_vector& buf, const std::string_view& string) noexcept {
    MetricBuffer::store_number(buf, string.size());
    buf.insert(buf.end(), string.begin(), string.end());
  }

  void store_msg(buf_vector& buf) const noexcept {
    MetricBuffer::store_number(buf, this->msg_size);
    MetricBuffer::store_string(buf, std::string_view{this->metric_name.c_str(), this->metric_name.size()});
    for (const auto& [tag_name, tag_value] : this->tags) {
      MetricBuffer::store_string(buf, std::string_view{tag_name.c_str(), tag_name.size()});
      MetricBuffer::store_string(buf, std::string_view{tag_value.c_str(), tag_value.size()});
    }
  }

  static size_t string_sizeof(const std::string_view& string) noexcept {
    return sizeof(size_t) + string.size();
  }

public:
  static MetricBuffer builder(std::string_view metric_name) noexcept {
    return MetricBuffer(metric_name);
  }

  // -----------------------------------------------------------------
  static MetricBuffer from_php(const string& metric_name, const array<string>& tags) noexcept {
    auto builder = MetricBuffer::builder({metric_name.c_str(), metric_name.size()});
    for (const auto& it : tags) {
      if (it.is_string_key()) {
        const auto& tag_key = it.get_string_key();
        const auto& tag_val = it.get_value();
        builder.set_tag({tag_key.c_str(), tag_key.size()}, {tag_val.c_str(), tag_val.size()});
      }
    }
    return builder;
  }
  // -----------------------------------------------------------------

  MetricBuffer& set_tag(std::string_view tag_name, std::string_view tag_value) noexcept {
    this->tags[string{tag_name.data(), static_cast<string::size_type>(tag_name.size())}] =
        string{tag_value.data(), static_cast<string::size_type>(tag_value.size())};
    this->msg_size += MetricBuffer::string_sizeof(tag_name) + MetricBuffer::string_sizeof(tag_value);
    return *this;
  }

  buf_vector build_value(double value, uint32_t timestamp) const noexcept {
    buf_vector buf{};
    buf.reserve(sizeof(uint32_t) + sizeof(uint32_t) + sizeof(double) + sizeof(size_t) + this->msg_size);

    MetricBuffer::store_number(buf, timestamp);
    MetricBuffer::store_number(buf, static_cast<uint32_t>(k2::MetricValueMask::VALUE_MASK));
    MetricBuffer::store_number(buf, value);
    this->store_msg(buf);
    return buf;
  }

  buf_vector build_values_array(const kphp::stl::vector<double, kphp::memory::script_allocator>& values, uint32_t timestamp) const noexcept {
    buf_vector buf{};
    buf.reserve(sizeof(uint32_t) + sizeof(uint32_t) + sizeof(size_t) + sizeof(double) * values.size() + sizeof(size_t) + this->msg_size);

    MetricBuffer::store_number(buf, timestamp);
    MetricBuffer::store_number(buf, static_cast<uint32_t>(k2::MetricValueMask::VALUES_ARRAY_MASK));
    MetricBuffer::store_number(buf, values.size());
    for (const auto& value : values) {
      MetricBuffer::store_number(buf, value);
    }
    this->store_msg(buf);
    return buf;
  }

  buf_vector build_count(double count, uint32_t timestamp) const noexcept {
    buf_vector buf{};
    buf.reserve(sizeof(uint32_t) + sizeof(uint32_t) + sizeof(double) + sizeof(size_t) + this->msg_size);

    MetricBuffer::store_number(buf, timestamp);
    MetricBuffer::store_number(buf, static_cast<uint32_t>(k2::MetricValueMask::COUNT_MASK));
    MetricBuffer::store_number(buf, count);
    this->store_msg(buf);
    return buf;
  }

  buf_vector build_increment(uint32_t timestamp) const noexcept {
    buf_vector buf{};
    buf.reserve(sizeof(uint32_t) + sizeof(uint32_t) + sizeof(size_t) + this->msg_size);

    MetricBuffer::store_number(buf, timestamp);
    MetricBuffer::store_number(buf, static_cast<uint32_t>(k2::MetricValueMask::INC_MASK));
    this->store_msg(buf);
    return buf;
  }
};

// -----------------------------------------------------------------

inline void f$write_metric_value(const string& metric_name, const array<string>& tags, double value, int64_t monitoring_system = 0) noexcept {
  auto builder = MetricBuffer::from_php(metric_name, tags);
  auto timestamp{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()};
  k2::write_serialized_metric(builder.build_value(value, static_cast<uint32_t>(timestamp)), static_cast<k2::MonitoringSystem>(monitoring_system));
}

inline void f$write_metric_values_array(const string& metric_name, const array<string>& tags, const array<double>& values,
                                        int64_t monitoring_system = 0) noexcept {
  auto builder = MetricBuffer::from_php(metric_name, tags);
  auto timestamp{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()};
  kphp::stl::vector<double, kphp::memory::script_allocator> vec{};
  for (const auto& it : values) {
    vec.push_back(it.get_value());
  }
  k2::write_serialized_metric(builder.build_values_array(vec, static_cast<uint32_t>(timestamp)), static_cast<k2::MonitoringSystem>(monitoring_system));
}

inline void f$write_metric_count(const string& metric_name, const array<string>& tags, double count, int64_t monitoring_system = 0) noexcept {
  auto builder = MetricBuffer::from_php(metric_name, tags);
  auto timestamp{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()};
  k2::write_serialized_metric(builder.build_count(count, static_cast<uint32_t>(timestamp)), static_cast<k2::MonitoringSystem>(monitoring_system));
}

inline void f$write_metric_increment(const string& metric_name, const array<string>& tags, int64_t monitoring_system = 0) noexcept {
  auto builder = MetricBuffer::from_php(metric_name, tags);
  auto timestamp{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()};
  k2::write_serialized_metric(builder.build_increment(static_cast<uint32_t>(timestamp)), static_cast<k2::MonitoringSystem>(monitoring_system));
}

inline void f$flush_metrics(int64_t monitoring_system = 0) noexcept {
  k2::flush_metrics(static_cast<k2::MonitoringSystem>(monitoring_system));
}
