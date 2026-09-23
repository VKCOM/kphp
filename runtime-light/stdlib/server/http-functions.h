// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/server/url-functions.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/server/http/http-server-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/fork/fork-functions.h"
#include "runtime-light/streams/connection.h"

namespace kphp::http {

void header(std::string_view header, bool replace, int64_t response_code) noexcept;

kphp::coro::task<> invoke_headers_callback(HttpServerInstanceState& state) noexcept;

// When finish is true, also drains user buffers, finishes compression and closes the response stream.
kphp::coro::task<> send_response(HttpServerInstanceState& state, bool finish) noexcept;

} // namespace kphp::http

inline void f$header(const string& str, bool replace = true, int64_t response_code = kphp::http::status::NO_STATUS) noexcept {
  kphp::http::header({str.c_str(), str.size()}, replace, response_code);
}

void f$setrawcookie(const string& name, const string& value, int64_t expire_or_options = 0, const string& path = {}, const string& domain = {},
                    bool secure = false, bool http_only = false) noexcept;

inline void f$setcookie(const string& name, const string& value = {}, int64_t expire_or_options = 0, const string& path = {}, const string& domain = {},
                        bool secure = false, bool http_only = false) noexcept {
  f$setrawcookie(name, f$urlencode(value), expire_or_options, path, domain, secure, http_only);
}

inline array<string> f$headers_list() noexcept {
  const auto& headers{HttpServerInstanceState::get().headers()};
  constexpr std::string_view header_separator{": "};

  array<string> list{array_size{static_cast<int64_t>(headers.size()), true}};
  for (const auto& [header_name, header_value] : headers) {
    list.push_back(string{static_cast<string::size_type>(header_name.size() + header_value.size() + header_separator.size()), true}
                       .append(header_name.c_str())
                       .append(header_separator.data())
                       .append(header_value.c_str()));
  }

  return list;
}

inline bool f$headers_sent([[maybe_unused]] Optional<std::optional<std::reference_wrapper<string>>> filename = {},
                           [[maybe_unused]] Optional<std::optional<std::reference_wrapper<string>>> line = {}) noexcept {
  return HttpServerInstanceState::get().headers_sent;
}

template<std::invocable F>
bool f$header_register_callback(F&& f) noexcept {
  auto& http_server_instance_st{HttpServerInstanceState::get()};
  if (http_server_instance_st.headers_callback_started || http_server_instance_st.headers_sent) {
    return false;
  }

  auto headers_callback_task{std::invoke(
      [](F f) noexcept -> kphp::coro::task<> {
        if constexpr (kphp::coro::is_async_function_v<F>) {
          co_await std::invoke(std::move(f));
        } else {
          std::invoke(std::move(f));
        }
      },
      std::forward<F>(f))};

  http_server_instance_st.headers_registered_callback.emplace(std::move(headers_callback_task));
  return true;
}

inline void f$send_http_103_early_hints([[maybe_unused]] const array<string>& headers) noexcept {
  // noop
}

inline kphp::coro::task<> f$flush() noexcept {
  auto& state{HttpServerInstanceState::get()};
  if (!state.connection.has_value()) {
    kphp::log::warning("immediate HTTP response available only from HTTP worker");
    co_return;
  }
  if (state.connection->is_aborted() || state.response_finished) {
    co_return;
  }
  co_await kphp::forks::id_managed(kphp::http::invoke_headers_callback(state));
  co_await kphp::forks::id_managed(kphp::http::send_response(state, /* finish = */ false));
}
