// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <variant>

#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/web-transfer-lib/defs.h"
#include "runtime-light/stdlib/web-transfer-lib/web-state.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"

namespace kphp::web::details {

inline auto process_error(const tl::WebError&& e) noexcept -> error {
  switch (e.code.value) {
  case tl_parse_error_code::server_side:
  case internal_error_code::unknown_backend:
  case internal_error_code::unknown_method:
  case internal_error_code::unknown_transfer:
  case backend_internal_error::cannot_take_handler_ownership:
  case backend_internal_error::post_field_value_not_string:
  case backend_internal_error::header_line_not_string:
  case backend_internal_error::unsupported_property:
    return error{.code = WEB_INTERNAL_ERROR_CODE, .description = string(e.description.value.data(), e.description.value.size())};
  default:
    // BackendError
    return error{.code = e.code.value, .description = string(e.description.value.data(), e.description.value.size())};
  }
}

} // namespace kphp::web::details

namespace kphp::web {

inline auto property_value::serialize() const noexcept -> tl::webPropertyValue {
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

inline auto property_value::deserialize(const tl::webPropertyValue& tl_prop_value) noexcept -> property_value {
  return std::visit(
      [](const auto& v) noexcept -> property_value {
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
              res[0] = std::get<0>(std::move(first_elem));
              for (int64_t i = 1; i < size; ++i) {
                res[i] = std::get<0>(deserialize(v.inner.value[i]).value);
              }
              return property_value::as_array_of_boolean(std::move(res));
            } else if (std::holds_alternative<int64_t>(first_elem)) {
              array<int64_t> res{array_size{size, true}};
              res[0] = std::get<1>(std::move(first_elem));
              for (int64_t i = 1; i < size; ++i) {
                res[i] = std::get<1>(deserialize(v.inner.value[i]).value);
              }
              return property_value::as_array_of_long(std::move(res));
            } else if (std::holds_alternative<double>(first_elem)) {
              array<double> res{array_size{size, true}};
              res[0] = std::get<1>(std::move(first_elem));
              for (int64_t i = 1; i < size; ++i) {
                res[i] = std::get<2>(deserialize(v.inner.value[i]).value);
              }
              return property_value::as_array_of_double(std::move(res));
            } else if (std::holds_alternative<string>(first_elem)) {
              array<string> res{array_size{size, true}};
              res[0] = std::get<3>(std::move(first_elem));
              for (int64_t i = 1; i < size; ++i) {
                res[i] = std::get<3>(deserialize(v.inner.value[i]).value);
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
              res[first_elem_key] = std::get<0>(std::move(first_elem_val));
              for (int64_t i = 1; i < size; ++i) {
                const auto key{string{v.inner.value.value[i].key.value.data(), static_cast<string::size_type>(v.inner.value.value[i].key.value.size())}};
                const auto val{std::get<0>(deserialize(v.inner.value.value[i].value).value)};
                res[key] = std::move(val);
              }
              return property_value::as_array_of_boolean(std::move(res));
            } else if (std::holds_alternative<int64_t>(first_elem_val)) {
              array<int64_t> res{array_size{size, false}};
              res[first_elem_key] = std::get<1>(std::move(first_elem_val));
              for (int64_t i = 1; i < size; ++i) {
                const auto key{string{v.inner.value.value[i].key.value.data(), static_cast<string::size_type>(v.inner.value.value[i].key.value.size())}};
                const auto val{std::get<1>(deserialize(v.inner.value.value[i].value).value)};
                res[key] = std::move(val);
              }
              return property_value::as_array_of_long(std::move(res));
            } else if (std::holds_alternative<double>(first_elem_val)) {
              array<double> res{array_size{size, false}};
              res[first_elem_key] = std::get<2>(std::move(first_elem_val));
              for (int64_t i = 1; i < size; ++i) {
                const auto key{string{v.inner.value.value[i].key.value.data(), static_cast<string::size_type>(v.inner.value.value[i].key.value.size())}};
                const auto val{std::get<2>(deserialize(v.inner.value.value[i].value).value)};
                res[key] = std::move(val);
              }
              return property_value::as_array_of_double(std::move(res));
            } else if (std::holds_alternative<string>(first_elem_val)) {
              array<string> res{array_size{size, false}};
              res[first_elem_key] = std::get<3>(std::move(first_elem_val));
              for (int64_t i = 1; i < size; ++i) {
                const auto key{string{v.inner.value.value[i].key.value.data(), static_cast<string::size_type>(v.inner.value.value[i].key.value.size())}};
                const auto val{std::get<3>(deserialize(v.inner.value.value[i].value).value)};
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

inline auto set_transfer_property(std::variant<simple_transfer, composite_transfer> transfer, property_id prop_id,
                                  property_value prop_value) -> std::expected<void, error> {
  // Simple
  if (std::holds_alternative<simple_transfer>(transfer)) {
    const auto simple{std::get<0>(transfer)};
    auto& simple2config{WebInstanceState::get().simple_transfer2config};
    if (!simple2config.contains(simple.descriptor)) {
      return std::unexpected{error{.code = WEB_INTERNAL_ERROR_CODE, .description = string("Unknown simple transfer id")}};
    }
    if (!simple2config[simple.descriptor].properties.contains(prop_id)) {
      simple2config[simple.descriptor].properties.insert({prop_id, std::move(prop_value)});
    } else {
      simple2config[simple.descriptor].properties.at(prop_id) = std::move(prop_value);
    }
    return std::expected<void, error>{};
  }
  // Composite
  const auto composite{std::get<1>(transfer)};
  auto& composite2config{WebInstanceState::get().composite_transfer2config};
  if (!composite2config.contains(composite.descriptor)) {
    return std::unexpected{error{.code = WEB_INTERNAL_ERROR_CODE, .description = string("Unknown composite transfer id")}};
  }
  if (!composite2config[composite.descriptor].properties.contains(prop_id)) {
    composite2config[composite.descriptor].properties.insert({prop_id, std::move(prop_value)});
  } else {
    composite2config[composite.descriptor].properties.at(prop_id) = std::move(prop_value);
  }
  return std::expected<void, error>{};
}

inline auto get_transfer_properties(std::variant<simple_transfer, composite_transfer> transfer,
                                    std::optional<property_id> prop_id) -> kphp::coro::task<std::expected<properties_type, error>> {
  // Try to get a cached prop
  if (prop_id.has_value()) {
    const auto p{prop_id.value()};
    const auto& web_state{WebInstanceState::get()};
    if (std::holds_alternative<simple_transfer>(transfer)) {
      auto s{std::get<0>(transfer).descriptor};
      if (web_state.simple_transfer2config.contains(s) && web_state.simple_transfer2config.at(s).properties.contains(p)) {
        properties_type res{};
        res.insert({p, web_state.simple_transfer2config.at(s).properties.at(p)});
        co_return std::expected<properties_type, error>{std::move(res)};
      }
    } else {
      auto c{std::get<1>(transfer).descriptor};
      if (web_state.composite_transfer2config.contains(c) && web_state.composite_transfer2config.at(c).properties.contains(p)) {
        properties_type res{};
        res.insert({p, web_state.composite_transfer2config.at(c).properties.at(p)});
        co_return std::expected<properties_type, error>{std::move(res)};
      }
    }
  }

  auto session{WebInstanceState::get().session_get_or_init()};
  if (!session.has_value()) {
    kphp::log::error("failed to start or get session with Web component");
  }

  if (session.value().get() == nullptr) {
    kphp::log::error("session with Web components has been closed");
  }

  tl::WebTransferGetProperties web_transfer_get_properties_req{
      .is_simple = tl::u8{std::holds_alternative<simple_transfer>(transfer)},
      .descriptor = tl::u64{(std::holds_alternative<simple_transfer>(transfer)) ? std::get<0>(transfer).descriptor : std::get<1>(transfer).descriptor},
      .property_id = (prop_id.has_value()) ? tl::Maybe<tl::u64>{tl::u64{prop_id.value()}} : tl::Maybe<tl::u64>{std::nullopt}};
  tl::storer tls{web_transfer_get_properties_req.footprint()};
  web_transfer_get_properties_req.store(tls);

  kphp::stl::vector<std::byte, kphp::memory::script_allocator> resp_buf{};
  auto response_buffer_provider{[&resp_buf](size_t size) noexcept -> std::span<std::byte> {
    resp_buf.resize(size);
    return {resp_buf.data(), size};
  }};

  auto resp{co_await session.value().get()->client.query(tls.view(), std::move(response_buffer_provider))};
  if (!resp.has_value()) [[unlikely]] {
    kphp::log::error("failed to send request for a getting of web properties");
  }

  tl::Either<tl::WebTransferGetPropertiesResultOk, tl::WebError> get_transfer_props_resp{};
  tl::fetcher tlf{resp.value()};
  if (!get_transfer_props_resp.fetch(tlf)) [[unlikely]] {
    kphp::log::error("failed to parse response with web properties");
  }
  if (const auto& r{std::move(get_transfer_props_resp.value)}; std::holds_alternative<tl::WebTransferGetPropertiesResultOk>(r)) {
    properties_type props{};
    const auto& tl_props{std::get<0>(r).properties};
    for (const auto& p : tl_props) {
      const auto k{p.id.value};
      const auto v{property_value::deserialize(std::move(p.value))};
      props.insert({k, std::move(v)});
    }
    if (prop_id.has_value() && !props.contains(prop_id.value())) {
      co_return std::unexpected{error{.code = WEB_INTERNAL_ERROR_CODE, .description = string("failed to find a specified property id")}};
    }
    co_return std::expected<properties_type, error>{std::move(props)};
  } else {
    co_return std::unexpected{details::process_error(std::get<1>(std::move(r)))};
  }
}

} // namespace kphp::web
