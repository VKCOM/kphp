// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/server/url-functions.h"
#include "runtime-light/server/http/http-server-state.h"

namespace kphp {

namespace http {

void header(std::string_view header, bool replace, int64_t response_code) noexcept;

} // namespace http

} // namespace kphp

inline void f$header(const string &str, bool replace = true, int64_t response_code = kphp::http::status::no_status) noexcept {
  kphp::http::header({str.c_str(), str.size()}, replace, response_code);
}

void f$setrawcookie(const string &name, const string &value, int64_t expire_or_options = 0, const string &path = {}, const string &domain = {},
                    bool secure = false, bool http_only = false) noexcept;

inline void f$setcookie(const string &name, const string &value = {}, int64_t expire_or_options = 0, const string &path = {}, const string &domain = {},
                        bool secure = false, bool http_only = false) noexcept {
  f$setrawcookie(name, f$urlencode(value), expire_or_options, path, domain, secure, http_only);
}
