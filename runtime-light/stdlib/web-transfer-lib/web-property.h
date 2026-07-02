// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <expected>
#include <optional>
#include <span>
#include <type_traits>
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

namespace kphp::web::property {

template<typename Transfer>
requires std::same_as<std::remove_cvref_t<Transfer>, simple::transfer> || std::same_as<std::remove_cvref_t<Transfer>, composite::transfer>
inline auto set(Transfer transfer, property::id prop_id, property::value prop_value) -> std::expected<void, error> {
  // Simple
  if constexpr (std::is_same_v<std::remove_cvref_t<Transfer>, simple::transfer>) {
    auto& simple2config{WebInstanceState::get().simple_transfer2config};
    if (!simple2config.contains(transfer.descriptor)) {
      return std::unexpected{error{.code = WEB_INTERNAL_ERROR_CODE, .description = string("Unknown simple transfer id")}};
    }
    auto& props{simple2config[transfer.descriptor].properties};
    if (const auto& prop{props.find(prop_id)}; prop != props.end()) {
      prop->second = std::move(prop_value);
    } else {
      props.emplace(prop_id, std::move(prop_value));
    }
    return std::expected<void, error>{};
  }
  // Composite
  auto& composite2config{WebInstanceState::get().composite_transfer2config};
  if (!composite2config.contains(transfer.descriptor)) {
    return std::unexpected{error{.code = WEB_INTERNAL_ERROR_CODE, .description = string("Unknown composite transfer id")}};
  }
  auto& props{composite2config[transfer.descriptor].properties};
  if (const auto& prop{props.find(prop_id)}; prop != props.end()) {
    prop->second = std::move(prop_value);
  } else {
    props.emplace(prop_id, std::move(prop_value));
  }
  return std::expected<void, error>{};
}

template<typename Transfer>
requires std::same_as<std::remove_cvref_t<Transfer>, simple::transfer> || std::same_as<std::remove_cvref_t<Transfer>, composite::transfer>
inline auto get(Transfer transfer, std::optional<property::id> prop_id, get_policy policy) -> kphp::coro::task<std::expected<properties_type, error>> {
  // Try to get a cached prop
  if (prop_id.has_value() && policy == get_policy::cached) {
    const auto p{prop_id.value()};
    const auto& web_state{WebInstanceState::get()};
    if constexpr (std::is_same_v<std::remove_cvref_t<Transfer>, simple::transfer>) {
      const auto& config{web_state.simple_transfer2config.find(transfer.descriptor)};
      if (config != web_state.simple_transfer2config.end()) {
        const auto& props{config->second.properties};
        if (const auto& prop{props.find(p)}; prop != props.end()) {
          properties_type res{};
          res.emplace(p, prop->second);
          co_return std::expected<properties_type, error>{std::move(res)};
        }
      }
    } else {
      const auto& config{web_state.composite_transfer2config.find(transfer.descriptor)};
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

  tl::WebTransferGetProperties web_transfer_get_properties_req{.is_simple = tl::u8{std::is_same_v<std::remove_cvref_t<Transfer>, simple::transfer>},
                                                               .descriptor = tl::u64{transfer.descriptor},
                                                               .property_id = (prop_id.has_value()) ? tl::Maybe<tl::u64>{tl::u64{(*prop_id)}}
                                                                                                    : tl::Maybe<tl::u64>{std::nullopt}};
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
      auto k{p.id.value};
      auto v{property::value::deserialize(p.value)};
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

} // namespace kphp::web::property
