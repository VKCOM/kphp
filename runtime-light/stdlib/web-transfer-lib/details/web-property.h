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

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/web-transfer-lib/defs.h"
#include "runtime-light/stdlib/web-transfer-lib/details/web-error.h"
#include "runtime-light/stdlib/web-transfer-lib/web-state.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"

namespace kphp::web {

inline auto set_transfer_property(std::variant<simple_transfer, composite_transfer> transfer, property_id prop_id,
                                  property_value prop_value) -> std::expected<void, error> {
  // Simple
  if (std::holds_alternative<simple_transfer>(transfer)) {
    const auto simple{std::get<simple_transfer>(transfer)};
    auto& simple2config{WebInstanceState::get().simple_transfer2config};
    if (!simple2config.contains(simple.descriptor)) {
      return std::unexpected{error{.code = WEB_INTERNAL_ERROR_CODE, .description = string("Unknown simple transfer id")}};
    }
    auto& props{simple2config[simple.descriptor].properties};
    if (const auto& prop{props.find(prop_id)}; prop != props.end()) {
      prop->second = std::move(prop_value);
    } else {
      props.emplace(prop_id, std::move(prop_value));
    }
    return std::expected<void, error>{};
  }
  // Composite
  const auto composite{std::get<composite_transfer>(transfer)};
  auto& composite2config{WebInstanceState::get().composite_transfer2config};
  if (!composite2config.contains(composite.descriptor)) {
    return std::unexpected{error{.code = WEB_INTERNAL_ERROR_CODE, .description = string("Unknown composite transfer id")}};
  }
  auto& props{composite2config[composite.descriptor].properties};
  if (const auto& prop{props.find(prop_id)}; prop != props.end()) {
    prop->second = std::move(prop_value);
  } else {
    props.emplace(prop_id, std::move(prop_value));
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
      auto s{std::get<simple_transfer>(transfer).descriptor};
      const auto& config{web_state.simple_transfer2config.find(s)};
      if (config != web_state.simple_transfer2config.end()) {
        const auto& props{config->second.properties};
        if (const auto& prop{props.find(p)}; prop != props.end()) {
          properties_type res{};
          res.emplace(p, prop->second);
          co_return std::expected<properties_type, error>{std::move(res)};
        }
      }
    } else {
      auto c{std::get<composite_transfer>(transfer).descriptor};
      const auto& config{web_state.composite_transfer2config.find(c)};
      if (config != web_state.composite_transfer2config.end()) {
        const auto& props{config->second.properties};
        if (const auto& prop{props.find(p)}; prop != props.end()) {
          properties_type res{};
          res.emplace(p, prop->second);
          co_return std::expected<properties_type, error>{std::move(res)};
        }
      }
    }
  }

  auto session{WebInstanceState::get().session_get_or_init()};
  if (!session.has_value()) {
    kphp::log::error("failed to start or get session with Web component");
  }

  if ((*session).get() == nullptr) {
    kphp::log::error("session with Web components has been closed");
  }

  tl::WebTransferGetProperties web_transfer_get_properties_req{
      .is_simple = tl::u8{std::holds_alternative<simple_transfer>(transfer)},
      .descriptor = tl::u64{(std::holds_alternative<simple_transfer>(transfer)) ? std::get<simple_transfer>(transfer).descriptor
                                                                                : std::get<composite_transfer>(transfer).descriptor},
      .property_id = (prop_id.has_value()) ? tl::Maybe<tl::u64>{tl::u64{(*prop_id)}} : tl::Maybe<tl::u64>{std::nullopt}};
  tl::storer tls{web_transfer_get_properties_req.footprint()};
  web_transfer_get_properties_req.store(tls);

  kphp::stl::vector<std::byte, kphp::memory::script_allocator> resp_buf{};
  auto response_buffer_provider{[&resp_buf](size_t size) noexcept -> std::span<std::byte> {
    resp_buf.resize(size);
    return {resp_buf.data(), size};
  }};

  auto resp{co_await (*session).get()->client.query(tls.view(), std::move(response_buffer_provider))};
  if (!resp.has_value()) [[unlikely]] {
    kphp::log::error("failed to send request for a getting of web properties");
  }

  tl::Either<tl::WebTransferGetPropertiesResultOk, tl::WebError> get_transfer_props_resp{};
  tl::fetcher tlf{*resp};
  if (!get_transfer_props_resp.fetch(tlf)) [[unlikely]] {
    kphp::log::error("failed to parse response with web properties");
  }
  if (auto r{std::move(get_transfer_props_resp.value)}; std::holds_alternative<tl::WebTransferGetPropertiesResultOk>(r)) {
    properties_type props{};
    auto& tl_props{std::get<tl::WebTransferGetPropertiesResultOk>(r).properties};
    for (const auto& p : tl_props) {
      const auto k{p.id.value};
      const auto v{property_value::deserialize(std::move(p.value))};
      props.emplace(k, std::move(v));
    }
    if (prop_id.has_value() && !props.contains(*prop_id)) {
      co_return std::unexpected{error{.code = WEB_INTERNAL_ERROR_CODE, .description = string("failed to find a specified property id")}};
    }
    co_return std::expected<properties_type, error>{std::move(props)};
  } else {
    co_return std::unexpected{details::process_error(std::get<tl::WebError>(std::move(r)))};
  }
}

} // namespace kphp::web
