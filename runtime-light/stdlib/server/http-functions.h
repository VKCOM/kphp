// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstdint>
#include <string_view>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/server/url-functions.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/server/http/http-server-state.h"

namespace kphp::http {

void header(std::string_view header, bool replace, int64_t response_code) noexcept;

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
  const auto& http_server_instance_st{HttpServerInstanceState::get()};
  switch (http_server_instance_st.response_state) {
  case kphp::http::response_state::not_started:
  case kphp::http::response_state::sending_headers:
    return false;
  case kphp::http::response_state::headers_sent:
  case kphp::http::response_state::sending_body:
  case kphp::http::response_state::completed:
    return true;
  }
}

template<std::invocable F>
bool f$header_register_callback(F&& f) noexcept {
  auto& http_server_instance_st{HttpServerInstanceState::get()};
  switch (http_server_instance_st.response_state) {
  case kphp::http::response_state::not_started:
    break;
  case kphp::http::response_state::sending_headers:
  case kphp::http::response_state::headers_sent:
  case kphp::http::response_state::sending_body:
  case kphp::http::response_state::completed:
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
