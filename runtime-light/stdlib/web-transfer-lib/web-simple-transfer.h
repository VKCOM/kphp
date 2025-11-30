// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <__expected/unexpected.h>
#include <cstddef>
#include <expected>
#include <optional>
#include <span>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/web-transfer-lib/defs.h"
#include "runtime-light/stdlib/web-transfer-lib/details/web-error.h"
#include "runtime-light/stdlib/web-transfer-lib/details/web-response.h"
#include "runtime-light/stdlib/web-transfer-lib/web-composite-transfer.h"
#include "runtime-light/stdlib/web-transfer-lib/web-state.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"

namespace kphp::web {

inline auto simple_transfer_open(transfer_backend backend) noexcept -> kphp::coro::task<std::expected<simple_transfer, error>> {
  auto& web_state{WebInstanceState::get()};
  auto session{web_state.session_get_or_init()};
  if (!session.has_value()) [[unlikely]] {
    kphp::log::error("failed to start or get session with Web component");
  }

  if ((*session).get() == nullptr) [[unlikely]] {
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

  auto resp{co_await (*session).get()->client.query(tls.view(), std::move(response_buffer_provider))};
  if (!resp.has_value()) [[unlikely]] {
    kphp::log::error("failed to send request of Simple descriptor creation");
  }

  tl::Either<tl::SimpleWebTransferOpenResultOk, tl::WebError> transfer_open_resp{};
  tl::fetcher tlf{*resp};
  if (!transfer_open_resp.fetch(tlf)) [[unlikely]] {
    kphp::log::error("failed to parse response of Simple descriptor creation");
  }

  auto result{transfer_open_resp.value};
  if (std::holds_alternative<tl::WebError>(result)) {
    co_return std::unexpected{details::process_error(std::get<tl::WebError>(result))};
  }

  const auto descriptor{std::get<tl::SimpleWebTransferOpenResultOk>(result).descriptor.value};

  auto& simple2config{web_state.simple_transfer2config};
  kphp::log::assertion(simple2config.contains(descriptor) == false); // NOLINT
  simple2config.emplace(descriptor, simple_transfer_config{});

  auto& composite_holder{web_state.simple_transfer2holder};
  kphp::log::assertion(composite_holder.contains(descriptor) == false); // NOLINT
  composite_holder.emplace(descriptor, std::nullopt);

  co_return std::expected<simple_transfer, error>{descriptor};
}

inline auto simple_transfer_perform(simple_transfer st) noexcept -> kphp::coro::task<std::expected<response, error>> {
  auto& web_state{WebInstanceState::get()};

  auto& simple2config{web_state.simple_transfer2config};
  if (!simple2config.contains(st.descriptor)) [[unlikely]] {
    co_return std::unexpected{error{.code = WEB_INTERNAL_ERROR_CODE, .description = string{"unknown Simple transfer"}}};
  }

  kphp::stl::vector<tl::webProperty, kphp::memory::script_allocator> tl_props{};
  auto& props{simple2config[st.descriptor].properties};
  for (const auto& [id, val] : props) {
    tl::webProperty p{.id = tl::u64{id}, .value = val.serialize()};
    tl_props.emplace_back(std::move(p));
  }
  tl::simpleWebTransferConfig tl_config{tl::vector<tl::webProperty>{std::move(tl_props)}};
  tl::SimpleWebTransferPerform tl_perform{.descriptor = tl::u64{st.descriptor}, .config = std::move(tl_config)};
  tl::storer tls{tl_perform.footprint()};
  tl_perform.store(tls);

  co_return co_await details::process_simple_response(tls.view());
}

inline auto simple_transfer_get_response(simple_transfer st) noexcept -> kphp::coro::task<std::expected<response, error>> {
  auto& web_state{WebInstanceState::get()};

  if (!web_state.simple_transfer2config.contains(st.descriptor)) {
    co_return std::unexpected{error{.code = WEB_INTERNAL_ERROR_CODE, .description = string{"unknown Simple transfer"}}};
  }

  tl::SimpleWebTransferGetResponse web_transfer_get_resp{.descriptor = tl::u64{st.descriptor}};
  tl::storer tls{web_transfer_get_resp.footprint()};
  web_transfer_get_resp.store(tls);

  co_return co_await details::process_simple_response(tls.view());
}

inline auto simple_transfer_reset(simple_transfer st) noexcept -> kphp::coro::task<std::expected<void, error>> {
  auto& web_state{WebInstanceState::get()};

  if (!web_state.simple_transfer2config.contains(st.descriptor)) {
    co_return std::unexpected{error{.code = WEB_INTERNAL_ERROR_CODE, .description = string{"unknown Simple transfer"}}};
  }

  auto session{web_state.session_get_or_init()};
  if (!session.has_value()) [[unlikely]] {
    kphp::log::error("failed to start or get session with Web component");
  }

  if ((*session).get() == nullptr) [[unlikely]] {
    kphp::log::error("session with Web components has been closed");
  }

  tl::SimpleWebTransferReset web_transfer_reset{.descriptor = tl::u64{st.descriptor}};
  tl::storer tls{web_transfer_reset.footprint()};
  web_transfer_reset.store(tls);

  kphp::stl::vector<std::byte, kphp::memory::script_allocator> resp_buf{};
  auto response_buffer_provider{[&resp_buf](size_t size) noexcept -> std::span<std::byte> {
    resp_buf.resize(size);
    return {resp_buf.data(), size};
  }};

  auto resp{co_await (*session).get()->client.query(tls.view(), std::move(response_buffer_provider))};
  if (!resp.has_value()) [[unlikely]] {
    kphp::log::error("failed to send request of Simple descriptor resetting");
  }

  tl::Either<tl::SimpleWebTransferResetResultOk, tl::WebError> transfer_reset_resp{};
  tl::fetcher tlf{*resp};
  if (!transfer_reset_resp.fetch(tlf)) [[unlikely]] {
    kphp::log::error("failed to parse response of Simple descriptor resetting");
  }

  if (auto r{transfer_reset_resp.value}; std::holds_alternative<tl::WebError>(r)) {
    co_return std::unexpected{details::process_error(std::get<tl::WebError>(r))};
  }

  auto& simple2config{web_state.simple_transfer2config};
  if (simple2config.contains(st.descriptor)) [[likely]] {
    simple2config[st.descriptor] = simple_transfer_config{};
  }

  co_return std::expected<void, error>{};
}

inline auto simple_transfer_close(simple_transfer st) noexcept -> kphp::coro::task<std::expected<void, error>> {
  auto& web_state{WebInstanceState::get()};

  if (!web_state.simple_transfer2config.contains(st.descriptor)) {
    co_return std::unexpected{error{.code = WEB_INTERNAL_ERROR_CODE, .description = string{"unknown Simple transfer"}}};
  }

  auto session{web_state.session_get_or_init()};
  if (!session.has_value()) [[unlikely]] {
    kphp::log::error("failed to start or get session with Web component");
  }

  if ((*session).get() == nullptr) [[unlikely]] {
    kphp::log::error("session with Web components has been closed");
  }

  // Checking that Simple transfer is held by some Composite transfer
  auto& composite_holder{web_state.simple_transfer2holder[st.descriptor]};
  if (composite_holder.has_value()) {
    if (auto remove_res{
            co_await kphp::web::composite_transfer_remove(kphp::web::composite_transfer{*composite_holder}, kphp::web::simple_transfer{st.descriptor})};
        !remove_res.has_value()) {
      co_return std::move(remove_res);
    };
  }

  tl::SimpleWebTransferClose web_transfer_close{.descriptor = tl::u64{st.descriptor}};
  tl::storer tls{web_transfer_close.footprint()};
  web_transfer_close.store(tls);

  kphp::stl::vector<std::byte, kphp::memory::script_allocator> resp_buf{};
  auto response_buffer_provider{[&resp_buf](size_t size) noexcept -> std::span<std::byte> {
    resp_buf.resize(size);
    return {resp_buf.data(), size};
  }};

  auto resp{co_await (*session).get()->client.query(tls.view(), std::move(response_buffer_provider))};
  if (!resp.has_value()) [[unlikely]] {
    kphp::log::error("failed to send request of Simple descriptor closing");
  }

  tl::Either<tl::SimpleWebTransferCloseResultOk, tl::WebError> transfer_close_resp{};
  tl::fetcher tlf{*resp};
  if (!transfer_close_resp.fetch(tlf)) [[unlikely]] {
    kphp::log::error("failed to parse response of Simple descriptor closing");
  }

  if (auto& r{transfer_close_resp.value}; std::holds_alternative<tl::WebError>(r)) {
    co_return std::unexpected{details::process_error(std::get<tl::WebError>(r))};
  }

  co_return std::expected<void, error>{};
}

} // namespace kphp::web
