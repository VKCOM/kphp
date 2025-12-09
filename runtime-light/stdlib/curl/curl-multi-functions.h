// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/curl/curl-context.h"
#include "runtime-light/stdlib/curl/curl-state.h"
#include "runtime-light/stdlib/curl/defs.h"
#include "runtime-light/stdlib/curl/details/diagnostics.h"
#include "runtime-light/stdlib/fork/fork-functions.h"
#include "runtime-light/stdlib/web-transfer-lib/defs.h"
#include "runtime-light/stdlib/web-transfer-lib/details/web-property.h"
#include "runtime-light/stdlib/web-transfer-lib/web-composite-transfer.h"
#include "runtime-light/stdlib/web-transfer-lib/web-simple-transfer.h"

inline auto f$curl_multi_init() noexcept -> kphp::coro::task<kphp::web::curl::multi_type> {
  auto open_res{co_await kphp::forks::id_managed(kphp::web::composite_transfer_open(kphp::web::transfer_backend::CURL))};
  if (!open_res.has_value()) [[unlikely]] {
    kphp::web::curl::print_error("could not initialize a new curl multi handle", std::move(open_res.error()));
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
  if (!curl_state.easy_ctx.has(easy_id)) [[unlikely]] {
    co_return false;
  }
  auto& multi_ctx{curl_state.multi_ctx.get_or_init(multi_id)};
  auto& easy_ctx{curl_state.easy_ctx.get_or_init(easy_id)};
  easy_ctx.errors_reset();

  auto res{co_await kphp::forks::id_managed(kphp::web::composite_transfer_add(kphp::web::composite_transfer{multi_id}, kphp::web::simple_transfer{easy_id}))};
  if (!res.has_value()) [[unlikely]] {
    multi_ctx.set_errno(res.error().code, res.error().description);
    kphp::web::curl::print_error("could not add a curl easy handler into multi handle", std::move(res.error()));
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
    kphp::web::curl::print_error("could not remove a curl easy handler from multi handle", std::move(res.error()));
    co_return multi_ctx.error_code;
  }
  multi_ctx.set_errno(kphp::web::curl::CURLE::OK);
  co_return multi_ctx.error_code;
}

inline auto f$curl_multi_setopt(kphp::web::curl::multi_type multi_id, int64_t option, int64_t value) noexcept -> bool {
  auto& curl_state{CurlInstanceState::get()};
  if (!curl_state.multi_ctx.has(multi_id)) {
    return false;
  }
  auto& multi_ctx{curl_state.multi_ctx.get_or_init(multi_id)};
  auto ct{kphp::web::composite_transfer{.descriptor = multi_id}};

  switch (static_cast<kphp::web::curl::CURMLOPT>(option)) {
  // long options
  case kphp::web::curl::CURMLOPT::PIPELINING: {
    auto pipeling_type{static_cast<kphp::web::curl::CURLPIPE>(value)};
    switch (pipeling_type) {
    case kphp::web::curl::CURLPIPE::NOTHING:
    case kphp::web::curl::CURLPIPE::HTTP1:
    case kphp::web::curl::CURLPIPE::MULTIPLEX: {
      auto res{kphp::web::set_transfer_property(ct, option, kphp::web::property_value::as_long(static_cast<int64_t>(pipeling_type)))};
      if (!res.has_value()) [[unlikely]] {
        kphp::web::curl::print_error("could not set an mutli option", std::move(res.error()));
        return false;
      }
      multi_ctx.set_errno(kphp::web::curl::CURLME::OK);
      return true;
    }
    default:
      multi_ctx.set_errno(kphp::web::curl::CURLME::UNKNOWN_OPTION, "a libcurl function was given an incorrect PROXYTYPE kind");
      return false;
    }
  }
  case kphp::web::curl::CURMLOPT::MAXCONNECTS:
  case kphp::web::curl::CURMLOPT::MAX_HOST_CONNECTIONS:
  case kphp::web::curl::CURMLOPT::MAX_PIPELINE_LENGTH:
  case kphp::web::curl::CURMLOPT::MAX_TOTAL_CONNECTIONS: {
    auto res{kphp::web::set_transfer_property(ct, option, kphp::web::property_value::as_long(value))};
    if (!res.has_value()) [[unlikely]] {
      kphp::web::curl::print_error("could not set an multi option", std::move(res.error()));
      return false;
    }
    multi_ctx.set_errno(kphp::web::curl::CURLME::OK);
    return true;
  }
  default:
    multi_ctx.set_errno(kphp::web::curl::CURLME::UNKNOWN_OPTION);
    return false;
  }
}

inline auto f$curl_multi_exec(kphp::web::curl::multi_type multi_id, int64_t& still_running) noexcept -> kphp::coro::task<Optional<int64_t>> {
  auto& curl_state{CurlInstanceState::get()};
  if (!curl_state.multi_ctx.has(multi_id)) [[unlikely]] {
    co_return false;
  }
  auto res{co_await kphp::forks::id_managed(kphp::web::composite_transfer_perform(kphp::web::composite_transfer{multi_id}))};
  auto& multi_ctx{curl_state.multi_ctx.get_or_init(multi_id)};
  if (!res.has_value()) [[unlikely]] {
    multi_ctx.set_errno(res.error().code, res.error().description);
    kphp::web::curl::print_error("could not execute curl multi handle", std::move(res.error()));
    co_return multi_ctx.error_code;
  }
  still_running = (*res);
  multi_ctx.set_errno(kphp::web::curl::CURLE::OK);
  co_return multi_ctx.error_code;
}

inline auto f$curl_multi_getcontent(kphp::web::curl::easy_type easy_id) noexcept -> kphp::coro::task<Optional<string>> {
  auto& curl_state{CurlInstanceState::get()};
  if (!curl_state.easy_ctx.has(easy_id)) [[unlikely]] {
    co_return false;
  }
  auto& easy_ctx{curl_state.easy_ctx.get_or_init(easy_id)};
  if (easy_ctx.return_transfer) {
    auto res{co_await kphp::forks::id_managed(kphp::web::simple_transfer_get_response(kphp::web::simple_transfer{easy_id}))};
    if (!res.has_value()) [[unlikely]] {
      easy_ctx.set_errno(res.error().code, res.error().description);
      kphp::web::curl::print_error("could not get response of curl easy handle", std::move(res.error()));
      co_return false;
    }
    co_return std::move((*res).body);
  }
  co_return Optional<string>{};
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
    kphp::web::curl::print_error("could not close curl multi handle", std::move(res.error()));
    co_return;
  }
  multi_ctx.set_errno(kphp::web::curl::CURLE::OK);
  co_return;
}

inline auto f$curl_multi_strerror(kphp::web::curl::multi_type multi_id) noexcept -> Optional<string> {
  auto& curl_state{CurlInstanceState::get()};
  if (!curl_state.multi_ctx.has(multi_id)) [[unlikely]] {
    return {};
  }
  auto& multi_ctx{curl_state.multi_ctx.get_or_init(multi_id)};
  if (multi_ctx.error_code != static_cast<int64_t>(kphp::web::curl::CURLME::OK)) [[likely]] {
    const auto* const desc_data{reinterpret_cast<const char*>(multi_ctx.error_description.data())};
    const auto desc_size{static_cast<string::size_type>(multi_ctx.error_description.size())};
    return string{desc_data, desc_size};
  }
  return {};
}

inline auto f$curl_multi_errno(kphp::web::curl::multi_type multi_id) noexcept -> Optional<int64_t> {
  auto& curl_state{CurlInstanceState::get()};
  if (!curl_state.multi_ctx.has(multi_id)) [[unlikely]] {
    return false;
  }
  auto& multi_ctx{curl_state.multi_ctx.get_or_init(multi_id)};
  return multi_ctx.error_code;
}

inline auto f$curl_multi_select(kphp::web::curl::multi_type multi_id, double timeout = 1.0) noexcept -> kphp::coro::task<Optional<int64_t>> {
  auto& curl_state{CurlInstanceState::get()};
  if (!curl_state.multi_ctx.has(multi_id)) {
    co_return false;
  }
  auto& multi_ctx{curl_state.multi_ctx.get_or_init(multi_id)};
  auto res{co_await kphp::forks::id_managed(kphp::web::composite_transfer_wait_updates(
      kphp::web::composite_transfer{multi_id}, std::chrono::duration_cast<std::chrono::seconds>(std::chrono::duration<double>{timeout})))};
  if (!res.has_value()) [[unlikely]] {
    multi_ctx.set_errno(res.error().code, res.error().description);
    kphp::web::curl::print_error("could not select curl multi handle", std::move(res.error()));
    co_return multi_ctx.error_code;
  }
  co_return std::move(*res);
}

inline auto f$curl_multi_info_read(kphp::web::curl::multi_type multi_id,
                                   std::optional<std::reference_wrapper<int64_t>> msgs_in_queue = {}) noexcept -> kphp::coro::task<Optional<array<int64_t>>> {
  auto& curl_state{CurlInstanceState::get()};
  if (!curl_state.multi_ctx.has(multi_id)) {
    co_return false;
  }
  auto& multi_ctx{curl_state.multi_ctx.get_or_init(multi_id)};

  constexpr auto CURL_MULTI_INFO_READ_OPTION = 0;

  auto props{co_await kphp::forks::id_managed(
      kphp::web::get_transfer_properties(kphp::web::composite_transfer{multi_id}, CURL_MULTI_INFO_READ_OPTION, kphp::web::get_properties_policy::load))};
  if (!props.has_value()) [[unlikely]] {
    multi_ctx.set_errno(props.error().code, props.error().description);
    kphp::web::curl::print_error("could not get info message of multi handle", std::move(props.error()));
    if (msgs_in_queue.has_value()) {
      (*msgs_in_queue).get() = 0;
    }
    co_return false;
  }

  auto it_optional_info{(*props).find(CURL_MULTI_INFO_READ_OPTION)};
  if (it_optional_info == (*props).end()) [[unlikely]] {
    kphp::web::curl::print_error("incorrect format of multi info message", std::move(props.error()));
    if (msgs_in_queue.has_value()) {
      (*msgs_in_queue).get() = 0;
    }
    co_return false;
  }

  const auto& optional_info{it_optional_info->second.to<array<int64_t>>()};
  if (!optional_info.has_value()) [[unlikely]] {
    if (msgs_in_queue.has_value()) {
      (*msgs_in_queue).get() = 0;
    }
    co_return false;
  }

  // Message is empty
  const auto& info{*optional_info};
  if (info.size().size == 0) {
    if (msgs_in_queue.has_value()) {
      (*msgs_in_queue).get() = 0;
    }
    co_return false;
  }

  constexpr auto MSGS_IN_QUEUE = 0;
  constexpr auto MSG_IDX = 1;
  constexpr auto RESULT_IDX = 2;
  constexpr auto HANDLE_IDX = 3;

  array<int64_t> result{array_size{3, false}};
  if (info.has_key(MSGS_IN_QUEUE) && msgs_in_queue.has_value()) {
    (*msgs_in_queue).get() = info.get_value(MSGS_IN_QUEUE);
  }

  if (info.has_key(MSG_IDX)) {
    result.set_value(string{"msg"}, info.get_value(MSG_IDX));
  }

  if (info.has_key(RESULT_IDX)) {
    result.set_value(string{"result"}, info.get_value(RESULT_IDX));
  }

  if (info.has_key(HANDLE_IDX)) {
    auto easy_id{info.get_value(HANDLE_IDX)};
    if (curl_state.easy_ctx.has(easy_id)) {
      result.set_value(string{"handle"}, info.get_value(HANDLE_IDX));
    }
  }

  co_return std::move(result);
}
