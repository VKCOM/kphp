// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-light/coroutine/event.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/web/web-state.h"
#include "runtime-light/streams/stream.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"

namespace kphp::web {

// IMPORTANT!
// All layouts and constants have to be synchronized with platform's definitions
// DO NOT INTRODUCE ANY CHANGES WITHOUT PLATFORM-SIDE MODIFICATION
enum class TransferBackend : uint8_t { CURL = 1 };

using SimpleTransfer = uint64_t;
using PropertyId = uint64_t;

class PropertyValue {
public:
  PropertyValue() = delete;

private:
  explicit PropertyValue(bool v) noexcept
      : value(v){};
  explicit PropertyValue(int64_t v) noexcept
      : value(v){};
  explicit PropertyValue(string v) noexcept
      : value(v){};
  explicit PropertyValue(array<bool> v) noexcept
      : value(v){};
  explicit PropertyValue(array<int64_t> v) noexcept
      : value(v){};
  explicit PropertyValue(array<string> v) noexcept
      : value(v){};

  std::variant<bool, int64_t, string, array<bool>, array<int64_t>, array<string>> value;

public:
  static inline auto as_boolean(int64_t value) -> PropertyValue {
    return PropertyValue{value};
  }

  static inline auto as_long(int64_t value) -> PropertyValue {
    return PropertyValue{value};
  }

  static inline auto as_string(string value) -> PropertyValue {
    return PropertyValue{std::move(value)};
  }

  static inline auto as_array_of_boolean(array<bool> value) -> PropertyValue {
    return PropertyValue{std::move(value)};
  }

  static inline auto as_array_of_long(array<int64_t> value) -> PropertyValue {
    return PropertyValue{std::move(value)};
  }

  static inline auto as_array_of_string(array<string> value) -> PropertyValue {
    return PropertyValue{std::move(value)};
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
                  res.inner.value.emplace_back(PropertyValue::as_boolean(i.get_value()).serialize());
                } else if constexpr (std::is_same_v<value_t, array<int64_t>>) {
                  res.inner.value.emplace_back(PropertyValue::as_long(i.get_value()).serialize());
                } else if constexpr (std::is_same_v<value_t, array<string>>) {
                  res.inner.value.emplace_back(PropertyValue::as_string(i.get_value()).serialize());
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
                      tl::dictionaryField{.key = tl::string{{key.c_str(), key.size()}}, .value = PropertyValue::as_boolean(val).serialize()});
                } else if constexpr (std::is_same_v<value_t, array<int64_t>>) {
                  res.inner.value.value.emplace_back(
                      tl::dictionaryField{.key = tl::string{{key.c_str(), key.size()}}, .value = PropertyValue::as_long(val).serialize()});
                } else if constexpr (std::is_same_v<value_t, array<string>>) {
                  res.inner.value.value.emplace_back(
                      tl::dictionaryField{.key = tl::string{{key.c_str(), key.size()}}, .value = PropertyValue::as_string(val).serialize()});
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

struct SimpleTransferConfig {
  kphp::stl::unordered_map<PropertyId, PropertyValue, kphp::memory::script_allocator> properties{};
};

struct Response {
  string headers;
  string body;
};

constexpr auto WEB_INTERNAL_ERROR_CODE = -1;

struct Error {
  int64_t code;
  std::optional<string> description;
};

enum TLParseErrorCode : int64_t {
  ServerSide = -1024,
};

enum InternalErrorCode : int64_t {
  UnknownBackend = -2048,
  UnknownMethod = -4096,
  UnknownTransfer = -8192,
};

enum BackendInternalError : int64_t {
  CannotTakeHandlerOwnership = -512,
  PostFieldValueNotString = -513,
  HeaderLineNotString = -514,
  UnsupportedProperty = -515,
};

} // namespace kphp::web
