// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <expected>
#include <span>
#include <utility>

#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/web-transfer-lib/defs.h"
#include "runtime-light/stdlib/web-transfer-lib/details/web-error.h"
#include "runtime-light/stdlib/web-transfer-lib/web-state.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"

namespace kphp::web {

inline auto composite_transfer_open(transfer_backend backend) noexcept -> kphp::coro::task<std::expected<composite_transfer, error>> {
  auto session{WebInstanceState::get().session_get_or_init()};
  if (!session.has_value()) [[unlikely]] {
    kphp::log::error("failed to start or get session with Web component");
  }

  if ((*session).get() == nullptr) [[unlikely]] {
    kphp::log::error("session with Web components has been closed");
  }

  tl::CompositeWebTransferOpen web_transfer_open{.web_backend = {static_cast<tl::CompositeWebTransferOpen::web_backend_type::underlying_type>(backend)}};
  tl::storer tls{web_transfer_open.footprint()};
  web_transfer_open.store(tls);

  kphp::stl::vector<std::byte, kphp::memory::script_allocator> resp_buf{};
  const auto response_buffer_provider{[&resp_buf](size_t size) noexcept -> std::span<std::byte> {
    resp_buf.resize(size);
    return {resp_buf.data(), size};
  }};

  auto resp{co_await (*session).get()->client.query(tls.view(), std::move(response_buffer_provider))};
  if (!resp.has_value()) [[unlikely]] {
    kphp::log::error("failed to send request for Composite descriptor creation");
  }

  tl::Either<tl::CompositeWebTransferOpenResultOk, tl::WebError> transfer_open_resp{};
  tl::fetcher tlf{*resp};
  if (!transfer_open_resp.fetch(tlf)) [[unlikely]] {
    kphp::log::error("failed to parse response for Composite descriptor creation");
  }

  const auto result{transfer_open_resp.value};
  if (std::holds_alternative<tl::WebError>(result)) {
    co_return std::unexpected{details::process_error(std::get<tl::WebError>(result))};
  }

  const auto descriptor{std::get<0>(result).descriptor.value};

  auto& composite2config{WebInstanceState::get().composite_transfer2config};
  kphp::log::assertion(composite2config.contains(descriptor) == false); // NOLINT
  composite2config.emplace(descriptor, composite_transfer_config{});

  co_return std::expected<composite_transfer, error>{descriptor};
}

} // namespace kphp::web
