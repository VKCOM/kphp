// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/server/url-functions.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/server/http/http-server-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

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
  return HttpServerInstanceState::get().response_state >= kphp::http::response_state::headers_sent;
}

template<typename F>
bool f$header_register_callback(F&& f) noexcept {
  auto& http_server_instance_st{HttpServerInstanceState::get()};
  if (http_server_instance_st.response_state >= kphp::http::response_state::headers_sent) [[unlikely]] {
    // don't save handler since it will not be called
    return false;
  } else if (http_server_instance_st.headers_custom_handler_invoked) [[unlikely]] {
    return false;
  }

  auto custom_header_handler_task{std::invoke(
      [](F f) noexcept -> kphp::coro::task<> {
        if constexpr (kphp::coro::is_async_function_v<F>) {
          co_await std::invoke(std::move(f));
        } else {
          std::invoke(std::move(f));
        }
      },
      std::forward<F>(f))};

  http_server_instance_st.headers_custom_handler_function = std::move(custom_header_handler_task);
  return true;
}

inline void f$send_http_103_early_hints([[maybe_unused]] const array<string>& headers) noexcept {
  // noop
}
