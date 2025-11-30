// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <functional>
#include <optional>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/curl/curl-context.h"
#include "runtime-light/stdlib/curl/curl-state.h"
#include "runtime-light/stdlib/curl/defs.h"
#include "runtime-light/stdlib/curl/details/diagnostics.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/fork/fork-functions.h"
#include "runtime-light/stdlib/web-transfer-lib/defs.h"
#include "runtime-light/stdlib/web-transfer-lib/web-composite-transfer.h"

inline auto f$curl_multi_init() noexcept -> kphp::coro::task<kphp::web::curl::multi_type> {
  auto open_res{co_await kphp::forks::id_managed(kphp::web::composite_transfer_open(kphp::web::transfer_backend::CURL))};
  if (!open_res.has_value()) [[unlikely]] {
    kphp::web::curl::print_error("Could not initialize a new curl multi handle", std::move(open_res.error()));
    co_return 0;
  }
  const auto descriptor{(*open_res).descriptor};
  CurlInstanceState::get().multi_ctx.get_or_init(descriptor);
  co_return descriptor;
}

inline auto f$curl_multi_add_handle(kphp::web::curl::multi_type multi_id, kphp::web::curl::easy_type easy_id) noexcept -> kphp::coro::task<Optional<int64_t>> {
  auto& curl_state{CurlInstanceState::get()};
  if (!curl_state.multi_ctx.has(multi_id)) [[unlikely]] {
    co_return false;
  }
  auto& multi_ctx{curl_state.multi_ctx.get_or_init(multi_id)};

  if (!curl_state.easy_ctx.has(easy_id)) [[unlikely]] {
    co_return false;
  }
  auto& easy_ctx{curl_state.easy_ctx.get_or_init(easy_id)};
  easy_ctx.errors_reset();

  auto res{co_await kphp::forks::id_managed(kphp::web::composite_transfer_add(kphp::web::composite_transfer{multi_id}, kphp::web::simple_transfer{easy_id}))};
  if (!res.has_value()) [[unlikely]] {
    multi_ctx.set_errno(res.error().code, res.error().description);
    kphp::web::curl::print_error("Could not add a curl easy handler into multi handle", std::move(res.error()));
    co_return multi_ctx.error_code;
  }
  multi_ctx.set_errno(kphp::web::curl::CURLE::OK);
  co_return multi_ctx.error_code;
}

inline auto f$curl_multi_remove_handle(kphp::web::curl::multi_type multi_id,
                                       kphp::web::curl::easy_type easy_id) noexcept -> kphp::coro::task<Optional<int64_t>> {
  auto& curl_state{CurlInstanceState::get()};
  if (!curl_state.multi_ctx.has(multi_id)) [[unlikely]] {
    co_return false;
  }
  auto& multi_ctx{curl_state.multi_ctx.get_or_init(multi_id)};
  if (!curl_state.easy_ctx.has(easy_id)) [[unlikely]] {
    co_return false;
  }
  auto res{
      co_await kphp::forks::id_managed(kphp::web::composite_transfer_remove(kphp::web::composite_transfer{multi_id}, kphp::web::simple_transfer{easy_id}))};
  if (!res.has_value()) [[unlikely]] {
    multi_ctx.set_errno(res.error().code, res.error().description);
    kphp::web::curl::print_error("Could not remove a curl easy handler from multi handle", std::move(res.error()));
    co_return multi_ctx.error_code;
  }
  multi_ctx.set_errno(kphp::web::curl::CURLE::OK);
  co_return multi_ctx.error_code;
}

inline auto f$curl_multi_close(kphp::web::curl::multi_type multi_id) noexcept -> kphp::coro::task<void> {
  auto& curl_state{CurlInstanceState::get()};
  if (!curl_state.multi_ctx.has(multi_id)) [[unlikely]] {
    co_return;
  }
  auto& multi_ctx{curl_state.multi_ctx.get_or_init(multi_id)};
  auto res{co_await kphp::forks::id_managed(kphp::web::composite_transfer_close(kphp::web::composite_transfer{multi_id}))};
  if (!res.has_value()) [[unlikely]] {
    multi_ctx.set_errno(res.error().code, res.error().description);
    kphp::web::curl::print_error("Could not close curl multi handle", std::move(res.error()));
    co_return;
  }
  multi_ctx.set_errno(kphp::web::curl::CURLE::OK);
  co_return;
}

inline Optional<array<int64_t>> f$curl_multi_info_read([[maybe_unused]] int64_t /*unused*/,
                                                       [[maybe_unused]] Optional<std::optional<std::reference_wrapper<int64_t>>> /*unused*/ = {}) {
  kphp::log::error("call to unsupported function : curl_multi_info_read");
}
