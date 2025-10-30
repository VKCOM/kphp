// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <utility>

#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/tl/tl-types.h"

namespace kphp::web {

// IMPORTANT!
// All layouts and constants have to be synchronized with platform's definitions
// DO NOT INTRODUCE ANY CHANGES WITHOUT PLATFORM-SIDE MODIFICATION
enum class transfer_backend : uint8_t { CURL = 1 };

using simple_transfer = uint64_t;
using property_id = uint64_t;

class property_value {
public:
  property_value() = delete;

private:
  explicit property_value(bool v) noexcept
      : value(v){};
  explicit property_value(int64_t v) noexcept
      : value(v){};
  explicit property_value(string v) noexcept
      : value(v){};
  explicit property_value(array<bool> v) noexcept
      : value(v){};
  explicit property_value(array<int64_t> v) noexcept
      : value(v){};
  explicit property_value(array<string> v) noexcept
      : value(v){};

  std::variant<bool, int64_t, string, array<bool>, array<int64_t>, array<string>> value;

public:
  static inline auto as_boolean(bool value) noexcept -> property_value {
    return property_value{value};
  }

  static inline auto as_long(int64_t value) noexcept -> property_value {
    return property_value{value};
  }

  static inline auto as_string(string value) noexcept -> property_value {
    return property_value{std::move(value)};
  }

  static inline auto as_array_of_boolean(array<bool> value) noexcept -> property_value {
    return property_value{std::move(value)};
  }

  static inline auto as_array_of_long(array<int64_t> value) noexcept -> property_value {
    return property_value{std::move(value)};
  }

  static inline auto as_array_of_string(array<string> value) noexcept -> property_value {
    return property_value{std::move(value)};
  }

  inline auto serialize() const noexcept -> tl::WebPropertyValue {
    return std::visit(
        [](const auto& v) noexcept -> tl::WebPropertyValue {
          using value_t = std::remove_cvref_t<decltype(v)>;
          if constexpr (std::is_same_v<value_t, bool>) {
            return tl::WebPropertyValue{tl::Bool{v}};
          } else if constexpr (std::is_same_v<value_t, int64_t>) {
            return tl::WebPropertyValue{tl::I64{tl::i64{v}}};
          } else if constexpr (std::is_same_v<value_t, string>) {
            return tl::WebPropertyValue{tl::String{tl::string{{v.c_str(), v.size()}}}};
          } else if constexpr (std::is_same_v<value_t, array<bool>> || std::is_same_v<value_t, array<int64_t>> || std::is_same_v<value_t, array<string>>) {
            if (v.is_vector()) {
              tl::Vector<tl::WebPropertyValue> res{tl::vector<tl::WebPropertyValue>{.value = {}}};
              for (const auto& i : v) {
                if constexpr (std::is_same_v<value_t, array<bool>>) {
                  res.inner.value.emplace_back(property_value::as_boolean(i.get_value()).serialize());
                } else if constexpr (std::is_same_v<value_t, array<int64_t>>) {
                  res.inner.value.emplace_back(property_value::as_long(i.get_value()).serialize());
                } else if constexpr (std::is_same_v<value_t, array<string>>) {
                  res.inner.value.emplace_back(property_value::as_string(i.get_value()).serialize());
                }
              }
              return tl::WebPropertyValue{.value = std::move(res)};
            } else {
              tl::Dictionary<tl::WebPropertyValue> res{
                  tl::Dictionary<tl::WebPropertyValue>{tl::vector<tl::dictionaryField<tl::WebPropertyValue>>{.value = {}}}};
              for (const auto& i : v) {
                // We cannot convert key into string right here since string which is produced will be destroyed once we occur out of this scope
                kphp::log::assertion(i.get_key().is_string());
                const auto& key{i.get_key().as_string()};
                const auto& val{i.get_value()};
                if constexpr (std::is_same_v<value_t, array<bool>>) {
                  res.inner.value.value.emplace_back(
                      tl::dictionaryField{.key = tl::string{{key.c_str(), key.size()}}, .value = property_value::as_boolean(val).serialize()});
                } else if constexpr (std::is_same_v<value_t, array<int64_t>>) {
                  res.inner.value.value.emplace_back(
                      tl::dictionaryField{.key = tl::string{{key.c_str(), key.size()}}, .value = property_value::as_long(val).serialize()});
                } else if constexpr (std::is_same_v<value_t, array<string>>) {
                  res.inner.value.value.emplace_back(
                      tl::dictionaryField{.key = tl::string{{key.c_str(), key.size()}}, .value = property_value::as_string(val).serialize()});
                }
              }
              return tl::WebPropertyValue{.value = std::move(res)};
            }
          } else {
            static_assert(false);
          }
        },
        this->value);
  }
};

struct simple_transfer_config {
  kphp::stl::unordered_map<property_id, property_value, kphp::memory::script_allocator> properties{};
};

struct response {
  string headers;
  string body;
};

constexpr auto WEB_INTERNAL_ERROR_CODE = -1;

struct error {
  int64_t code;
  std::optional<string> description;
};

enum tl_parse_error_code : int64_t {
  server_side = -1024,
};

enum internal_error_code : int64_t {
  unknown_backend = -2048,
  unknown_method = -4096,
  unknown_transfer = -8192,
};

enum backend_internal_error : int64_t {
  cannot_take_handler_ownership = -512,
  post_field_value_not_string = -513,
  header_line_not_string = -514,
  unsupported_property = -515,
};

} // namespace kphp::web
