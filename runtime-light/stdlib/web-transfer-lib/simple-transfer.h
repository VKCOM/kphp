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
#include "runtime-light/stdlib/web-transfer-lib/web.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"

namespace kphp::web {

inline auto simple_transfer_open(transfer_backend backend) noexcept -> kphp::coro::task<std::expected<simple_transfer, error>> {
  auto session{WebInstanceState::get().session_get_or_init()};
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

  const auto result{transfer_open_resp.value};
  if (std::holds_alternative<tl::WebError>(result)) {
    co_return std::unexpected{details::process_error(std::get<1>(std::move(result)))};
  }

  const auto descriptor{std::get<0>(result).descriptor.value};

  auto& simple2config{WebInstanceState::get().simple_transfer2config};
  kphp::log::assertion(simple2config.contains(descriptor) == false);
  simple2config.emplace(descriptor, simple_transfer_config{});

  co_return std::expected<simple_transfer, error>{descriptor};
}

inline auto simple_transfer_perform(simple_transfer st) noexcept -> kphp::coro::task<std::expected<response, error>> {
  auto& web_state{WebInstanceState::get()};
  auto& simple2config{web_state.simple_transfer2config};
  if (!simple2config.contains(st.descriptor)) [[unlikely]] {
    kphp::log::error("unknown Simple descriptor");
  }

  auto session{web_state.session_get_or_init()};
  if (!session.has_value()) [[unlikely]] {
    kphp::log::error("failed to start or get session with Web component");
  }

  if ((*session).get() == nullptr) [[unlikely]] {
    kphp::log::error("session with Web components has been closed");
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
      if (auto r{simple_web_transfer_perform_resp.value}; std::holds_alternative<tl::WebError>(r)) {
        err.emplace(details::process_error(std::get<1>(std::move(r))));
        return false;
      }
      return true;
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

  if (auto res{co_await (*session).get()->client.query(tls.view(), std::move(response_buffer_provider), response_handler)}; !res) [[unlikely]] {
    kphp::log::error("failed to send request of Simple descriptor performing");
  }
  if (err.has_value()) [[unlikely]] {
    co_return std::unexpected{std::move(*err)};
  }
  co_return std::expected<response, error>{std::move(resp)};
}

inline auto simple_transfer_reset(simple_transfer st) noexcept -> kphp::coro::task<std::expected<void, error>> {
  auto session{WebInstanceState::get().session_get_or_init()};
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

  if (const auto& r{transfer_reset_resp.value}; std::holds_alternative<tl::WebError>(r)) {
    co_return std::unexpected{details::process_error(std::get<1>(std::move(r)))};
  }

  auto& simple2config{WebInstanceState::get().simple_transfer2config};
  if (simple2config.contains(st.descriptor)) [[likely]] {
    simple2config[st.descriptor] = simple_transfer_config{};
  }

  co_return std::expected<void, error>{};
}

inline auto simple_transfer_close(simple_transfer st) noexcept -> kphp::coro::task<std::expected<void, error>> {
  auto session{WebInstanceState::get().session_get_or_init()};
  if (!session.has_value()) [[unlikely]] {
    kphp::log::error("failed to start or get session with Web component");
  }

  if ((*session).get() == nullptr) [[unlikely]] {
    kphp::log::error("session with Web components has been closed");
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

  if (const auto& r{transfer_close_resp.value}; std::holds_alternative<tl::WebError>(r)) {
    co_return std::unexpected{details::process_error(std::get<1>(std::move(r)))};
  }

  co_return std::expected<void, error>{};
}

} // namespace kphp::web
