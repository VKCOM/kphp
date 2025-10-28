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

struct simple_transfer {
  using descriptor_type = uint64_t;
  descriptor_type descriptor;
};

struct composite_transfer {
  using descriptor_type = uint64_t;
  descriptor_type descriptor;
};

using property_id = uint64_t;

class property_value {
public:
  property_value() = delete;

private:
  explicit property_value(bool v) noexcept
      : value(v){};
  explicit property_value(int64_t v) noexcept
      : value(v){};
  explicit property_value(double v) noexcept
      : value(v){};
  explicit property_value(string v) noexcept
      : value(v){};
  explicit property_value(array<bool> v) noexcept
      : value(v){};
  explicit property_value(array<int64_t> v) noexcept
      : value(v){};
  explicit property_value(array<double> v) noexcept
      : value(v){};
  explicit property_value(array<string> v) noexcept
      : value(v){};

  std::variant<bool, int64_t, double, string, array<bool>, array<int64_t>, array<double>, array<string>> value;

public:
  static inline auto as_boolean(bool value) noexcept -> property_value {
    return property_value{value};
  }

  static inline auto as_long(int64_t value) noexcept -> property_value {
    return property_value{value};
  }

  static inline auto as_double(double value) noexcept -> property_value {
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

  static inline auto as_array_of_double(array<double> value) noexcept -> property_value {
    return property_value{std::move(value)};
  }

  static inline auto as_array_of_string(array<string> value) noexcept -> property_value {
    return property_value{std::move(value)};
  }

  inline auto serialize() const noexcept -> tl::webPropertyValue;
  static inline auto deserialize(const tl::webPropertyValue& tl_prop_value) noexcept -> property_value;

  inline auto to_mixed() const noexcept -> mixed;
};

using properties_type = kphp::stl::unordered_map<property_id, property_value, kphp::memory::script_allocator>;

struct simple_transfer_config {
  properties_type properties{};
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
