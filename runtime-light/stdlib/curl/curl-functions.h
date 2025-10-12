// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cstdint>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/component/inter-component-session/client.h"
#include "runtime-light/stdlib/curl/curl-options.h"
#include "runtime-light/stdlib/curl/curl-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/output/print-functions.h"
#include "runtime-light/stdlib/web/web.h"

using curl_easy = kphp::web::SimpleTransfer;
constexpr auto CURLE_UNKNOWN_OPTION = 48;

inline auto set_error(int64_t code, std::string_view description) noexcept {
  auto& ctx{CurlInstanceState::get()};
  ctx.error_code = code;
  std::memcpy(ctx.error_description.data(), description.data(), std::min(description.size(), static_cast<size_t>(CURL_ERROR_SIZE)));
}

inline auto set_error(int64_t code, std::optional<string> description) noexcept {
  if (description.has_value()) {
    set_error(code, description.value().c_str());
    return;
  }
  set_error(code, string().c_str());
}

template<size_t N>
inline auto bad_option_error(const char (&msg)[N], bool save_error = true) noexcept {
  static_assert(N <= CURL_ERROR_SIZE, "Too long error");
  kphp::log::warning("{}", msg);
  if (save_error) {
    set_error(CURLE_UNKNOWN_OPTION, {{msg, N}});
  }
}

template<size_t N>
inline auto print_error(const char (&msg)[N], kphp::web::Error&& error) noexcept {
  static_assert(N <= CURL_ERROR_SIZE, "Too long error");
  if (error.description.has_value()) {
    kphp::log::warning("{}\nCode: {}; Description: {}", msg, error.code, error.description.value().c_str());
  } else {
    kphp::log::warning("{}: {}", msg, error.code);
  }
}

inline bool f$curl_setopt_array(int64_t /*unused*/, const array<mixed>& /*unused*/) noexcept {
  kphp::log::error("call to unsupported function : curl_multi_info_read");
}

inline auto f$curl_init(const string url = string("")) noexcept -> kphp::coro::task<curl_easy> {
  auto res{co_await kphp::web::simple_transfer_open(kphp::web::TransferBackend::CURL)};
  if (!res.has_value()) {
    set_error(res.error().code, res.error().description);
    print_error("Could not initialize a new curl easy handle", std::move(res.error()));
    co_return 0;
  }
  kphp::web::set_transfer_prop(res.value(), CURLOPT::URL, kphp::web::PropertyValue::as_string(url));
  co_return res.value();
}

inline auto f$curl_setopt(curl_easy easy_id, int64_t option, const mixed& value) noexcept -> bool {
  if (option == CURLOPT::RETURNTRANSFER) {
    easyctx_get_or_init(easy_id).return_transfer = (value.to_int() == 1ll);
    return true;
  }

  switch (option) {
  case CURLOPT::ADDRESS_SCOPE:
  case CURLOPT::APPEND:
  case CURLOPT::AUTOREFERER:
  case CURLOPT::BUFFERSIZE:
  case CURLOPT::CONNECT_ONLY:
  case CURLOPT::CONNECTTIMEOUT:
  case CURLOPT::CONNECTTIMEOUT_MS:
  case CURLOPT::COOKIESESSION:
  case CURLOPT::CRLF:
  case CURLOPT::DIRLISTONLY:
  case CURLOPT::DNS_CACHE_TIMEOUT:
  case CURLOPT::FAILONERROR:
  case CURLOPT::FILETIME:
  case CURLOPT::FOLLOWLOCATION:
  case CURLOPT::FORBID_REUSE:
  case CURLOPT::FRESH_CONNECT:
  case CURLOPT::FTP_CREATE_MISSING_DIRS:
  case CURLOPT::FTP_RESPONSE_TIMEOUT:
  case CURLOPT::FTP_SKIP_PASV_IP:
  case CURLOPT::FTP_USE_EPRT:
  case CURLOPT::FTP_USE_EPSV:
  case CURLOPT::FTP_USE_PRET:
  case CURLOPT::HEADER:
  case CURLOPT::HTTP_CONTENT_DECODING:
  case CURLOPT::HTTP_TRANSFER_DECODING:
  case CURLOPT::HTTPGET:
  case CURLOPT::HTTPPROXYTUNNEL:
  case CURLOPT::IGNORE_CONTENT_LENGTH:
  case CURLOPT::INFILESIZE:
  case CURLOPT::LOW_SPEED_LIMIT:
  case CURLOPT::LOW_SPEED_TIME:
  case CURLOPT::MAXCONNECTS:
  case CURLOPT::MAXFILESIZE:
  case CURLOPT::MAXREDIRS:
  case CURLOPT::NETRC:
  case CURLOPT::NEW_DIRECTORY_PERMS:
  case CURLOPT::NEW_FILE_PERMS:
  case CURLOPT::NOBODY:
  case CURLOPT::PORT:
  case CURLOPT::POST:
  case CURLOPT::PROXY_TRANSFER_MODE:
  case CURLOPT::PROXYPORT:
  case CURLOPT::RESUME_FROM:
  case CURLOPT::SOCKS5_GSSAPI_NEC:
  case CURLOPT::SSL_SESSIONID_CACHE:
  case CURLOPT::SSL_VERIFYHOST:
  case CURLOPT::SSL_VERIFYPEER:
  case CURLOPT::TCP_NODELAY:
  case CURLOPT::TFTP_BLKSIZE:
  case CURLOPT::TIMEOUT:
  case CURLOPT::TIMEOUT_MS:
  case CURLOPT::TRANSFERTEXT:
  case CURLOPT::UNRESTRICTED_AUTH:
  case CURLOPT::UPLOAD:
  case CURLOPT::VERBOSE:
  case CURLOPT::WILDCARDMATCH:
  case CURLOPT::PUT:
  case CURLOPT::HTTP_VERSION:
  case CURLOPT::TCP_KEEPALIVE:
  case CURLOPT::TCP_KEEPIDLE:
  case CURLOPT::TCP_KEEPINTVL:
    return kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_long(value.to_int())).has_value();
  case CURLOPT::MAX_RECV_SPEED_LARGE:
  case CURLOPT::MAX_SEND_SPEED_LARGE:
    return kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_long(value.to_int())).has_value();
  case CURLOPT::CAINFO:
  case CURLOPT::CAPATH:
  case CURLOPT::COOKIE:
  case CURLOPT::COOKIEFILE:
  case CURLOPT::COOKIEJAR:
  case CURLOPT::COOKIELIST:
  case CURLOPT::CRLFILE:
  case CURLOPT::CUSTOMREQUEST:
  case CURLOPT::EGDSOCKET:
  case CURLOPT::FTP_ACCOUNT:
  case CURLOPT::FTP_ALTERNATIVE_TO_USER:
  case CURLOPT::FTPPORT:
  case CURLOPT::INTERFACE:
  case CURLOPT::ISSUERCERT:
  case CURLOPT::KRBLEVEL:
  case CURLOPT::MAIL_FROM:
  case CURLOPT::NETRC_FILE:
  case CURLOPT::NOPROXY:
  case CURLOPT::PASSWORD:
  case CURLOPT::PROXY:
  case CURLOPT::PROXYPASSWORD:
  case CURLOPT::PROXYUSERNAME:
  case CURLOPT::PROXYUSERPWD:
  case CURLOPT::RANDOM_FILE:
  case CURLOPT::RANGE:
  case CURLOPT::REFERER:
  case CURLOPT::RTSP_SESSION_ID:
  case CURLOPT::RTSP_STREAM_URI:
  case CURLOPT::RTSP_TRANSPORT:
  case CURLOPT::SOCKS5_GSSAPI_SERVICE:
  case CURLOPT::SSH_HOST_PUBLIC_KEY_MD5:
  case CURLOPT::SSH_KNOWNHOSTS:
  case CURLOPT::SSH_PRIVATE_KEYFILE:
  case CURLOPT::SSH_PUBLIC_KEYFILE:
  case CURLOPT::SSLCERT:
  case CURLOPT::SSLCERTTYPE:
  case CURLOPT::SSLENGINE:
  case CURLOPT::SSLENGINE_DEFAULT:
  case CURLOPT::SSLKEY:
  case CURLOPT::SSLKEYTYPE:
  case CURLOPT::SSL_CIPHER_LIST:
  case CURLOPT::URL:
  case CURLOPT::USERAGENT:
  case CURLOPT::USERNAME:
  case CURLOPT::USERPWD:
  case CURLOPT::ACCEPT_ENCODING:
    return kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_string(value.to_string())).has_value();
  case CURLOPT::HTTP200ALIASES:
  case CURLOPT::HTTPHEADER:
  case CURLOPT::POSTQUOTE:
  case CURLOPT::PREQUOTE:
  case CURLOPT::QUOTE:
  case CURLOPT::MAIL_RCPT:
  case CURLOPT::RESOLVE: {
    if (unlikely(!value.is_array())) {
      bad_option_error("Value must be an array in function curl_setopt\n");
      return false;
    }
    const auto& arr = value.as_array();
    if (arr.empty()) {
      return false;
    }
    array<string> arr_of_str{arr.size()};
    auto i{0};
    for (auto p : arr) {
      arr_of_str[i++] = f$strval(p.get_value());
    }
    return kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_array_of_string(arr_of_str)).has_value();
  }
  case CURLOPT::POSTFIELDS: {
    if (!value.is_array()) {
      kphp::web::set_transfer_prop(easy_id, CURLOPT::COPYPOSTFIELDS, kphp::web::PropertyValue::as_string(value.to_string()));
      return true;
    }
    const auto& arr = value.as_array();
    if (arr.empty()) {
      return false;
    }
    array<string> arr_of_str{arr.size()};
    for (auto p : arr) {
      const string k = f$strval(p.get_key());
      const string v = f$strval(p.get_value());
      if (!v.empty() && v[0] == '@') [[unlikely]] {
        bad_option_error("files posting is not supported in curl_setopt with CURLOPT_POSTFIELDS\n");
        return false;
      }
      arr_of_str[k] = v;
    }
    return kphp::web::set_transfer_prop(easy_id, CURLOPT::HTTPPOST, kphp::web::PropertyValue::as_array_of_string(arr_of_str)).has_value();
  }
  case CURLOPT::PROXYTYPE:
    return kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_long(value.to_int())).has_value();
  case CURLOPT::SSLVERSION:
    return kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_long(value.to_int())).has_value();
  case CURLOPT::HTTPAUTH:
  case CURLOPT::PROXYAUTH:
    return kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_long(value.to_int())).has_value();
  case CURLOPT::IPRESOLVE:
    return kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_long(value.to_int())).has_value();
  case CURLOPT::PRIVATE: {
    easyctx_get_or_init(easy_id).private_data = value.to_string();
    return true;
  }
  default:
    bad_option_error("Unknown option\n");
    return false;
  }
}

inline auto f$curl_exec(int64_t easy_id) noexcept -> kphp::coro::task<mixed> {
  auto res{co_await kphp::web::simple_transfer_perform(easy_id)};
  if (!res.has_value()) {
    set_error(res.error().code, res.error().description);
    print_error("Could not exec curl easy handle", std::move(res.error()));
    co_return false;
  }
  auto& ctx{CurlInstanceState::get()};
  if (ctx.easy2ctx.contains(easy_id) && ctx.easy2ctx[easy_id].return_transfer) {
    co_return res.value().body;
  }
  print(res.value().body);
  co_return true;
}
