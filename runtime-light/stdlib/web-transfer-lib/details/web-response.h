// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <__expected/expected.h>
#include <cstddef>
#include <expected>
#include <optional>
#include <span>
#include <utility>
#include <variant>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/web-transfer-lib/defs.h"
#include "runtime-light/stdlib/web-transfer-lib/details/web-error.h"
#include "runtime-light/stdlib/web-transfer-lib/web-state.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-types.h"

namespace kphp::web::details {

inline auto process_simple_response(std::span<const std::byte> request) noexcept -> kphp::coro::task<std::expected<response, error>> {
  auto& web_state{WebInstanceState::get()};

  auto session{web_state.session_get_or_init()};
  if (!session.has_value()) [[unlikely]] {
    kphp::log::error("failed to start or get session with Web component");
  }

  if ((*session).get() == nullptr) [[unlikely]] {
    kphp::log::error("session with Web components has been closed");
  }

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

  const auto response_handler{[&frame_num, &err, &ok_or_error_buffer](
                                  [[maybe_unused]] std::span<std::byte> _) noexcept -> kphp::component::inter_component_session::client::response_readiness {
    switch (frame_num) {
    case 0: {
      frame_num += 1;
      tl::Either<tl::SimpleWebTransferPerformResultOk, tl::WebError> simple_web_transfer_perform_resp{};
      tl::fetcher tlf{ok_or_error_buffer};
      if (!simple_web_transfer_perform_resp.fetch(tlf)) [[unlikely]] {
        kphp::log::error("failed to parse response of Simple descriptor");
      }
      if (auto r{simple_web_transfer_perform_resp.value}; std::holds_alternative<tl::WebError>(r)) {
        err.emplace(details::process_error(std::get<tl::WebError>(r)));
        return kphp::component::inter_component_session::client::response_readiness::ready;
      }
      return kphp::component::inter_component_session::client::response_readiness::pending;
    }
    case 1:
      frame_num += 1;
      return kphp::component::inter_component_session::client::response_readiness::pending;
    case 2: // NOLINT
      return kphp::component::inter_component_session::client::response_readiness::ready;
    default:
      return kphp::component::inter_component_session::client::response_readiness::ready;
    }
  }};

  if (auto res{co_await (*session).get()->client.query(request, std::move(response_buffer_provider), response_handler)}; !res) [[unlikely]] {
    kphp::log::error("failed to send request of Simple descriptor processing");
  }
  if (err.has_value()) [[unlikely]] {
    co_return std::unexpected{std::move(*err)};
  }
  co_return std::expected<response, error>{std::move(resp)};
}

} // namespace kphp::web::details
