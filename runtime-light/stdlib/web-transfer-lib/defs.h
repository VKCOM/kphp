// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <variant>

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

  template<class T>
  requires std::same_as<T, bool> || std::same_as<T, int64_t> || std::same_as<T, double> || std::same_as<T, string> || std::same_as<T, array<bool>> ||
               std::same_as<T, array<int64_t>> || std::same_as<T, array<double>> || std::same_as<T, array<string>>
  inline auto to() const noexcept -> std::optional<T>;
};

using simple_transfers = kphp::stl::unordered_set<kphp::web::simple_transfer::descriptor_type, kphp::memory::script_allocator>;

using properties_type = kphp::stl::unordered_map<property_id, property_value, kphp::memory::script_allocator>;

struct simple_transfer_config {
  properties_type properties{};
};

struct composite_transfer_config {
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
  cannot_set_transfer_token = -516,
};

// ----------------------------------

inline auto property_value::serialize() const noexcept -> tl::webPropertyValue { // NOLINT
  return std::visit(
      [](const auto& v) noexcept -> tl::webPropertyValue {
        using value_t = std::remove_cvref_t<decltype(v)>;
        if constexpr (std::is_same_v<value_t, bool>) {
          return tl::webPropertyValue{tl::Bool{v}};
        } else if constexpr (std::is_same_v<value_t, int64_t>) {
          return tl::webPropertyValue{tl::I64{tl::i64{v}}};
        } else if constexpr (std::is_same_v<value_t, double>) {
          return tl::webPropertyValue{tl::F64{tl::f64{v}}};
        } else if constexpr (std::is_same_v<value_t, string>) {
          return tl::webPropertyValue{tl::String{tl::string{{v.c_str(), v.size()}}}};
        } else if constexpr (std::is_same_v<value_t, array<bool>> || std::is_same_v<value_t, array<int64_t>> || std::is_same_v<value_t, array<double>> ||
                             std::is_same_v<value_t, array<string>>) {
          if (v.is_vector()) {
            tl::Vector<tl::webPropertyValue> res{tl::vector<tl::webPropertyValue>{.value = {}}};
            for (const auto& i : v) {
              if constexpr (std::is_same_v<value_t, array<bool>>) {
                res.inner.value.emplace_back(property_value::as_boolean(i.get_value()).serialize());
              } else if constexpr (std::is_same_v<value_t, array<int64_t>>) {
                res.inner.value.emplace_back(property_value::as_long(i.get_value()).serialize());
              } else if constexpr (std::is_same_v<value_t, array<double>>) {
                res.inner.value.emplace_back(property_value::as_double(i.get_value()).serialize());
              } else if constexpr (std::is_same_v<value_t, array<string>>) {
                res.inner.value.emplace_back(property_value::as_string(i.get_value()).serialize());
              }
            }
            return tl::webPropertyValue{.value = std::move(res)};
          } else {
            tl::Dictionary<tl::webPropertyValue> res{tl::Dictionary<tl::webPropertyValue>{tl::vector<tl::dictionaryField<tl::webPropertyValue>>{.value = {}}}};
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
              } else if constexpr (std::is_same_v<value_t, array<double>>) {
                res.inner.value.value.emplace_back(
                    tl::dictionaryField{.key = tl::string{{key.c_str(), key.size()}}, .value = property_value::as_double(val).serialize()});
              } else if constexpr (std::is_same_v<value_t, array<string>>) {
                res.inner.value.value.emplace_back(
                    tl::dictionaryField{.key = tl::string{{key.c_str(), key.size()}}, .value = property_value::as_string(val).serialize()});
              }
            }
            return tl::webPropertyValue{.value = std::move(res)};
          }
        } else {
          static_assert(false);
        }
      },
      this->value);
}

inline auto property_value::deserialize(const tl::webPropertyValue& tl_prop_value) noexcept -> property_value { // NOLINT
  return std::visit(
      [](const auto& v) noexcept -> property_value { // NOLINT
        using value_t = std::remove_cvref_t<decltype(v)>;
        if constexpr (std::is_same_v<value_t, tl::Bool>) {
          return property_value::as_boolean(v.value);
        } else if constexpr (std::is_same_v<value_t, tl::I64>) {
          return property_value::as_long(v.inner.value);
        } else if constexpr (std::is_same_v<value_t, tl::F64>) {
          return property_value::as_double(v.inner.value);
        } else if constexpr (std::is_same_v<value_t, tl::String>) {
          return property_value::as_string(string{v.inner.value.data(), static_cast<string::size_type>(v.inner.value.size())});
        } else if constexpr (std::is_same_v<value_t, tl::Vector<tl::webPropertyValue>>) {
          const auto size{static_cast<int64_t>(v.size())};
          if (size > 0) {
            const auto first_elem{deserialize(v.inner.value[0]).value};
            if (std::holds_alternative<bool>(first_elem)) {
              array<bool> res{array_size{size, true}};
              res[0] = std::get<bool>(std::move(first_elem));
              for (int64_t i = 1; i < size; ++i) {
                res[i] = std::get<bool>(deserialize(v.inner.value[i]).value);
              }
              return property_value::as_array_of_boolean(std::move(res));
            } else if (std::holds_alternative<int64_t>(first_elem)) {
              array<int64_t> res{array_size{size, true}};
              res[0] = std::get<int64_t>(std::move(first_elem));
              for (int64_t i = 1; i < size; ++i) {
                res[i] = std::get<int64_t>(deserialize(v.inner.value[i]).value);
              }
              return property_value::as_array_of_long(std::move(res));
            } else if (std::holds_alternative<double>(first_elem)) {
              array<double> res{array_size{size, true}};
              res[0] = std::get<double>(std::move(first_elem));
              for (int64_t i = 1; i < size; ++i) {
                res[i] = std::get<double>(deserialize(v.inner.value[i]).value);
              }
              return property_value::as_array_of_double(std::move(res));
            } else if (std::holds_alternative<string>(first_elem)) {
              array<string> res{array_size{size, true}};
              res[0] = std::get<string>(std::move(first_elem));
              for (int64_t i = 1; i < size; ++i) {
                res[i] = std::get<string>(deserialize(v.inner.value[i]).value);
              }
              return property_value::as_array_of_string(std::move(res));
            }
            kphp::log::assertion(false);
            return property_value::as_array_of_boolean({});
          }
          return property_value::as_array_of_boolean({});
        } else if constexpr (std::is_same_v<value_t, tl::Dictionary<tl::webPropertyValue>>) {
          const auto size{static_cast<int64_t>(v.size())};
          if (size > 0) {
            const auto first_elem_val{deserialize(v.inner.value.value[0].value).value};
            const auto first_elem_key{string{v.inner.value.value[0].key.value.data(), static_cast<string::size_type>(v.inner.value.value[0].key.value.size())}};
            if (std::holds_alternative<bool>(first_elem_val)) {
              array<bool> res{array_size{size, false}};
              res[first_elem_key] = std::get<bool>(std::move(first_elem_val));
              for (int64_t i = 1; i < size; ++i) {
                const auto key{string{v.inner.value.value[i].key.value.data(), static_cast<string::size_type>(v.inner.value.value[i].key.value.size())}};
                const auto val{std::get<bool>(deserialize(v.inner.value.value[i].value).value)};
                res[key] = std::move(val);
              }
              return property_value::as_array_of_boolean(std::move(res));
            } else if (std::holds_alternative<int64_t>(first_elem_val)) {
              array<int64_t> res{array_size{size, false}};
              res[first_elem_key] = std::get<int64_t>(std::move(first_elem_val));
              for (int64_t i = 1; i < size; ++i) {
                const auto key{string{v.inner.value.value[i].key.value.data(), static_cast<string::size_type>(v.inner.value.value[i].key.value.size())}};
                const auto val{std::get<int64_t>(deserialize(v.inner.value.value[i].value).value)};
                res[key] = std::move(val);
              }
              return property_value::as_array_of_long(std::move(res));
            } else if (std::holds_alternative<double>(first_elem_val)) {
              array<double> res{array_size{size, false}};
              res[first_elem_key] = std::get<double>(std::move(first_elem_val));
              for (int64_t i = 1; i < size; ++i) {
                const auto key{string{v.inner.value.value[i].key.value.data(), static_cast<string::size_type>(v.inner.value.value[i].key.value.size())}};
                const auto val{std::get<double>(deserialize(v.inner.value.value[i].value).value)};
                res[key] = std::move(val);
              }
              return property_value::as_array_of_double(std::move(res));
            } else if (std::holds_alternative<string>(first_elem_val)) {
              array<string> res{array_size{size, false}};
              res[first_elem_key] = std::get<string>(std::move(first_elem_val));
              for (int64_t i = 1; i < size; ++i) {
                const auto key{string{v.inner.value.value[i].key.value.data(), static_cast<string::size_type>(v.inner.value.value[i].key.value.size())}};
                const auto val{std::get<string>(deserialize(v.inner.value.value[i].value).value)};
                res[key] = std::move(val);
              }
              return property_value::as_array_of_string(std::move(res));
            }
            kphp::log::assertion(false);
            return property_value::as_array_of_boolean({});
          }
          return property_value::as_array_of_boolean({});
        } else {
          static_assert(false);
        }
      },
      tl_prop_value.value);
}

inline auto property_value::to_mixed() const noexcept -> mixed {
  return std::visit([](const auto& v) noexcept -> mixed { return mixed{v}; }, this->value);
}

template<class T>
requires std::same_as<T, bool> || std::same_as<T, int64_t> || std::same_as<T, double> || std::same_as<T, string> || std::same_as<T, array<bool>> ||
             std::same_as<T, array<int64_t>> || std::same_as<T, array<double>> || std::same_as<T, array<string>>
inline auto property_value::to() const noexcept -> std::optional<T> {
  if (!std::holds_alternative<T>(this->value)) {
    return {};
  }
  return std::get<T>(this->value);
}

} // namespace kphp::web
