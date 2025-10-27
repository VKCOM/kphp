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

#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/web-transfer-lib/defs.h"
#include "runtime-light/stdlib/web-transfer-lib/web-state.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"

namespace kphp::web {

namespace details {
inline auto process_error(tl::WebError&& e) noexcept -> error {
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

} // namespace details

inline auto simple_transfer_open(transfer_backend backend) noexcept -> kphp::coro::task<std::expected<simple_transfer, error>> {
  auto session{WebInstanceState::get().session_get_or_init()};
  if (!session.has_value()) [[unlikely]] {
    kphp::log::error("failed to start or get session with Web component");
  }

  if (session.value().get() == nullptr) [[unlikely]] {
    kphp::log::error("session with Web components has been closed");
  }

  tl::SimpleWebTransferOpen web_transfer_open{.web_backend = {static_cast<tl::SimpleWebTransferOpen::web_backend_type::underlying_type>(backend)}};
  tl::storer tls{web_transfer_open.footprint()};
  web_transfer_open.store(tls);

  kphp::stl::vector<std::byte, kphp::memory::script_allocator> resp_buf{};
  const auto response_buffer_provider{[&resp_buf](size_t size) noexcept -> std::span<std::byte> {
    resp_buf.resize(size);
    return {resp_buf.data(), size};
  }};

  auto resp{co_await session.value().get()->client.query(tls.view(), std::move(response_buffer_provider))};
  if (!resp.has_value()) [[unlikely]] {
    kphp::log::error("failed to send request of Simple descriptor creation");
  }

  tl::Either<tl::SimpleWebTransferOpenResultOk, tl::WebError> simple_web_transfer_resp{};
  tl::fetcher tlf{resp.value()};
  if (!simple_web_transfer_resp.fetch(tlf)) [[unlikely]] {
    kphp::log::error("failed to parse response of Simple descriptor creation");
  }

  std::expected<simple_transfer, error> result;
  std::visit(
      [&result](const auto& v) noexcept {
        using value_t = std::remove_cvref_t<decltype(v)>;
        if constexpr (std::same_as<value_t, tl::SimpleWebTransferOpenResultOk>) {
          result = std::expected<simple_transfer, error>{static_cast<tl::SimpleWebTransferOpenResultOk>(v).descriptor.value};
        } else {
          result = std::unexpected{details::process_error(static_cast<tl::WebError>(v))};
        }
      },
      simple_web_transfer_resp.value);

  if (!result.has_value()) {
    co_return std::move(result);
  }

  auto& simple2config{WebInstanceState::get().simple_transfer2config};
  kphp::log::assertion(simple2config.contains(result.value()) == false);
  simple2config.emplace(result.value(), simple_transfer_config{});

  co_return std::move(result);
}

inline auto set_transfer_prop(simple_transfer st, property_id prop_id, property_value prop_value) -> std::expected<void, error> {
  auto& simple2config{WebInstanceState::get().simple_transfer2config};
  if (!simple2config.contains(st)) {
    return std::unexpected{error{.code = WEB_INTERNAL_ERROR_CODE, .description = string("Unknown transfer id")}};
  }
  if (!simple2config[st].properties.contains(prop_id)) {
    simple2config[st].properties.insert({prop_id, std::move(prop_value)});
  } else {
    simple2config[st].properties.at(prop_id) = std::move(prop_value);
  }
  return std::expected<void, error>{};
}

inline auto simple_transfer_perform(simple_transfer st) noexcept -> kphp::coro::task<std::expected<response, error>> {
  auto& web_state{WebInstanceState::get()};
  auto& simple2config{web_state.simple_transfer2config};
  if (!simple2config.contains(st)) [[unlikely]] {
    kphp::log::error("unknown Simple descriptor");
  }

  auto session{web_state.session_get_or_init()};
  if (!session.has_value()) [[unlikely]] {
    kphp::log::error("failed to start or get session with Web component");
  }

  if (session.value().get() == nullptr) [[unlikely]] {
    kphp::log::error("session with Web components has been closed");
  }

  kphp::stl::vector<tl::WebProperty, kphp::memory::script_allocator> tl_props{};
  auto& props{simple2config[st].properties};
  for (const auto& [id, val] : props) {
    tl::WebProperty p{.id = tl::u64{id}, .value = val.serialize()};
    tl_props.emplace_back(std::move(p));
  }
  tl::SimpleWebTransferConfig tl_config{tl::vector<tl::WebProperty>{std::move(tl_props)}};
  tl::SimpleWebTransferPerform tl_perform{.desc = tl::u64{st}, .config = std::move(tl_config)};
  tl::storer tls{tl_perform.footprint()};
  tl_perform.store(tls);

  kphp::stl::vector<std::byte, kphp::memory::script_allocator> ok_or_error_buffer{};
  response resp{};
  std::optional<error> err{};
  auto frame_num{0};

  auto response_buffer_provider{[&frame_num, &ok_or_error_buffer, &resp](size_t size) noexcept -> std::span<std::byte> {
    switch (frame_num) {
    case 0:
      ok_or_error_buffer.resize(size);
      return {ok_or_error_buffer.data(), size};
    case 1:
      resp.headers = string{static_cast<string::size_type>(size), false};
      return {reinterpret_cast<std::byte*>(resp.headers.buffer()), size};
    case 2:
      resp.body = string{static_cast<string::size_type>(size), false};
      return {reinterpret_cast<std::byte*>(resp.body.buffer()), size};
    default:
      kphp::log::assertion(false);
      return {};
    }
  }};

  const auto response_handler{[&frame_num, &err, &ok_or_error_buffer](std::span<std::byte> _) noexcept -> bool {
    switch (frame_num) {
    case 0: {
      frame_num += 1;
      tl::Either<tl::SimpleWebTransferPerformResultOk, tl::WebError> simple_web_transfer_perform_resp{};
      tl::fetcher tlf{ok_or_error_buffer};
      if (!simple_web_transfer_perform_resp.fetch(tlf)) [[unlikely]] {
        kphp::log::error("failed to parse response of Simple descriptor performing");
      }
      std::visit(
          [&err](const auto& v) noexcept {
            using value_t = std::remove_cvref_t<decltype(v)>;
            if constexpr (std::same_as<value_t, tl::WebError>) {
              err.emplace(details::process_error(static_cast<tl::WebError>(v)));
            }
          },
          simple_web_transfer_perform_resp.value);
      return !err.has_value();
    }
    case 1:
      frame_num += 1;
      return true;
    case 2:
      return false;
    default:
      return false;
    }
  }};

  if (auto res{co_await session.value().get()->client.query(tls.view(), std::move(response_buffer_provider), response_handler)}; !res) [[unlikely]] {
    kphp::log::error("failed to send request of Simple descriptor performing");
  }
  if (err.has_value()) {
    co_return std::unexpected{std::move(err.value())};
  }
  co_return std::expected<response, error>{std::move(resp)};
}

[[maybe_unused]] inline auto simple_transfer_reset(simple_transfer st) noexcept -> kphp::coro::task<std::expected<void, error>> {
  co_return std::unexpected{error{.code = -1, .description = std::nullopt}};
}

inline auto simple_transfer_close(simple_transfer st) noexcept -> kphp::coro::task<std::expected<void, error>> {
  auto session{WebInstanceState::get().session_get_or_init()};
  if (!session.has_value()) {
    kphp::log::error("failed to start or get session with Web component");
  }

  if (session.value().get() == nullptr) {
    kphp::log::error("session with Web components has been closed");
  }

  tl::SimpleWebTransferClose web_transfer_close{.desc = tl::u64{st}};
  tl::storer tls{web_transfer_close.footprint()};
  web_transfer_close.store(tls);

  kphp::stl::vector<std::byte, kphp::memory::script_allocator> resp_buf{};
  auto response_buffer_provider{[&resp_buf](size_t size) noexcept -> std::span<std::byte> {
    resp_buf.resize(size);
    return {resp_buf.data(), size};
  }};

  auto resp{co_await session.value().get()->client.query(tls.view(), std::move(response_buffer_provider))};
  if (!resp.has_value()) [[unlikely]] {
    kphp::log::error("failed to send request of Simple descriptor closing");
  }

  tl::Either<tl::SimpleWebTransferCloseResultOk, tl::WebError> simple_web_transfer_resp{};
  tl::fetcher tlf{resp.value()};
  if (!simple_web_transfer_resp.fetch(tlf)) [[unlikely]] {
    kphp::log::error("failed to parse response of Simple descriptor closing");
  }

  std::expected<void, error> result;
  std::visit(
      [&result](const auto& v) noexcept {
        using value_t = std::remove_cvref_t<decltype(v)>;
        if constexpr (std::same_as<value_t, tl::WebError>) {
          result = std::unexpected{details::process_error(static_cast<tl::WebError>(v))};
        } else {
          result = std::expected<void, error>{};
        }
      },
      simple_web_transfer_resp.value);
  co_return std::move(result);
}

} // namespace kphp::web
