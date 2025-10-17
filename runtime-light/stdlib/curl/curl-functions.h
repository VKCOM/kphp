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

inline auto set_errno(int64_t code, std::string_view description) noexcept {
  // If Web Transfer Lib specific error
  if (code == kphp::web::WEB_INTERNAL_ERROR_CODE) [[unlikely]] {
    return;
  }
  auto& ctx{CurlInstanceState::get()};
  ctx.error_code = code;
  std::memcpy(ctx.error_description.data(), description.data(), std::min(description.size(), static_cast<size_t>(CURL_ERROR_SIZE)));
}

inline auto set_errno(int64_t code, std::optional<string> description = std::nullopt) noexcept {
  if (description.has_value()) {
    set_errno(code, description.value().c_str());
    return;
  }
  set_errno(code, string().c_str());
}

template<size_t N>
inline auto bad_option_error(const char (&msg)[N]) noexcept {
  static_assert(N <= CURL_ERROR_SIZE, "Too long error");
  kphp::log::warning("{}", msg);
  set_errno(CURLE::BAD_FUNCTION_ARGUMENT, {{msg, N}});
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

inline auto f$curl_init(const string url = string("")) noexcept -> kphp::coro::task<curl_easy> {
  auto open_res{co_await kphp::web::simple_transfer_open(kphp::web::TransferBackend::CURL)};
  if (!open_res.has_value()) [[unlikely]] {
    set_errno(open_res.error().code, open_res.error().description);
    print_error("Could not initialize a new curl easy handle", std::move(open_res.error()));
    co_return 0;
  }
  auto setopt_res{kphp::web::set_transfer_prop(open_res.value(), CURLOPT::URL, kphp::web::PropertyValue::as_string(url))};
  if (!setopt_res.has_value()) [[unlikely]] {
    set_errno(setopt_res.error().code, setopt_res.error().description);
    print_error("Could not set URL for a new curl easy handle", std::move(setopt_res.error()));
    co_return 0;
  }
  co_return open_res.value();
}

inline auto f$curl_setopt(curl_easy easy_id, int64_t option, const mixed& value) noexcept -> bool {
  if (option == PHPCURL::INFO_HEADER_OUT) {
    auto res{kphp::web::set_transfer_prop(easy_id, CURLOPT::VERBOSE, kphp::web::PropertyValue::as_long(value.to_int() == 1ll))};
    if (!res.has_value()) [[unlikely]] {
      print_error("Could not set an option", std::move(res.error()));
    }
    set_errno(CURLE::OK);
    return true;
  }

  if (option == PHPCURL::OPT_RETURNTRANSFER) {
    easyctx_get_or_init(easy_id).return_transfer = (value.to_int() == 1ll);
    set_errno(CURLE::OK);
    return true;
  }

  switch (option) {
  // "Long" options
  case CURLOPT::ADDRESS_SCOPE:
  // case CURLOPT::APPEND:
  case CURLOPT::AUTOREFERER:
  case CURLOPT::BUFFERSIZE:
  case CURLOPT::CONNECT_ONLY:
  case CURLOPT::CONNECTTIMEOUT:
  case CURLOPT::CONNECTTIMEOUT_MS:
  case CURLOPT::COOKIESESSION:
  // case CURLOPT::CRLF:
  // case CURLOPT::DIRLISTONLY:
  case CURLOPT::DNS_CACHE_TIMEOUT:
  case CURLOPT::FAILONERROR:
  case CURLOPT::FILETIME:
  case CURLOPT::FOLLOWLOCATION:
  case CURLOPT::FORBID_REUSE:
  case CURLOPT::FRESH_CONNECT:
  // case CURLOPT::FTP_CREATE_MISSING_DIRS:
  // case CURLOPT::FTP_RESPONSE_TIMEOUT:
  // case CURLOPT::FTP_SKIP_PASV_IP:
  // case CURLOPT::FTP_USE_EPRT:
  // case CURLOPT::FTP_USE_EPSV:
  // case CURLOPT::FTP_USE_PRET:
  case CURLOPT::HEADER:
  case CURLOPT::HTTP_CONTENT_DECODING:
  case CURLOPT::HTTP_TRANSFER_DECODING:
  case CURLOPT::HTTPGET:
  case CURLOPT::HTTPPROXYTUNNEL:
  case CURLOPT::IGNORE_CONTENT_LENGTH:
  // case CURLOPT::INFILESIZE:
  case CURLOPT::LOW_SPEED_LIMIT:
  case CURLOPT::LOW_SPEED_TIME:
  case CURLOPT::MAXCONNECTS:
  case CURLOPT::MAXFILESIZE:
  case CURLOPT::MAXREDIRS:
  // case CURLOPT::NEW_DIRECTORY_PERMS:
  // case CURLOPT::NEW_FILE_PERMS:
  case CURLOPT::NOBODY:
  case CURLOPT::PORT:
  case CURLOPT::POST:
  // case CURLOPT::PROXY_TRANSFER_MODE:
  case CURLOPT::PROXYPORT:
  // case CURLOPT::RESUME_FROM:
  // case CURLOPT::SOCKS5_GSSAPI_NEC:
  case CURLOPT::SSL_SESSIONID_CACHE:
  case CURLOPT::SSL_VERIFYHOST:
  case CURLOPT::SSL_VERIFYPEER:
  case CURLOPT::SSLENGINE_DEFAULT:
  case CURLOPT::TCP_NODELAY:
  // case CURLOPT::TFTP_BLKSIZE:
  case CURLOPT::TIMEOUT:
  case CURLOPT::TIMEOUT_MS:
  // case CURLOPT::TRANSFERTEXT:
  case CURLOPT::UNRESTRICTED_AUTH:
  case CURLOPT::UPLOAD:
  case CURLOPT::VERBOSE:
  case CURLOPT::WILDCARDMATCH:
  case CURLOPT::PUT:
  case CURLOPT::TCP_KEEPALIVE:
  case CURLOPT::TCP_KEEPIDLE:
  case CURLOPT::TCP_KEEPINTVL: {
    auto res{kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_long(value.to_int()))};
    if (!res.has_value()) [[unlikely]] {
      print_error("Could not set an option", std::move(res.error()));
      return false;
    }
    set_errno(CURLE::OK);
    return true;
  }
  // "off_t" options
  case CURLOPT::MAX_RECV_SPEED_LARGE:
  case CURLOPT::MAX_SEND_SPEED_LARGE: {
    auto res{kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_long(value.to_int()))};
    if (!res.has_value()) [[unlikely]] {
      print_error("Could not set an option", std::move(res.error()));
      return false;
    }
    set_errno(CURLE::OK);
    return true;
  }
  // "string" options
  case CURLOPT::CAINFO:
  case CURLOPT::CAPATH:
  case CURLOPT::COOKIE:
  case CURLOPT::COOKIEFILE:
  case CURLOPT::COOKIEJAR:
  case CURLOPT::COOKIELIST:
  case CURLOPT::CRLFILE:
  case CURLOPT::CUSTOMREQUEST:
  case CURLOPT::EGDSOCKET:
  // case CURLOPT::FTP_ACCOUNT:
  // case CURLOPT::FTP_ALTERNATIVE_TO_USER:
  // case CURLOPT::FTPPORT:
  case CURLOPT::INTERFACE:
  case CURLOPT::ISSUERCERT:
  // case CURLOPT::KRBLEVEL:
  // case CURLOPT::MAIL_FROM:
  // case CURLOPT::NETRC_FILE:
  case CURLOPT::NOPROXY:
  case CURLOPT::PASSWORD:
  case CURLOPT::PROXY:
  case CURLOPT::PROXYPASSWORD:
  case CURLOPT::PROXYUSERNAME:
  // case CURLOPT::PROXYUSERPWD:
  case CURLOPT::RANDOM_FILE:
  case CURLOPT::RANGE:
  case CURLOPT::REFERER:
  // case CURLOPT::RTSP_SESSION_ID:
  // case CURLOPT::RTSP_STREAM_URI:
  // case CURLOPT::RTSP_TRANSPORT:
  // case CURLOPT::SOCKS5_GSSAPI_SERVICE:
  // case CURLOPT::SSH_HOST_PUBLIC_KEY_MD5:
  // case CURLOPT::SSH_KNOWNHOSTS:
  // case CURLOPT::SSH_PRIVATE_KEYFILE:
  // case CURLOPT::SSH_PUBLIC_KEYFILE:
  case CURLOPT::SSLCERT:
  case CURLOPT::SSLCERTTYPE:
  case CURLOPT::SSLENGINE:
  case CURLOPT::SSLKEY:
  case CURLOPT::SSLKEYTYPE:
  case CURLOPT::SSL_CIPHER_LIST:
  case CURLOPT::URL:
  case CURLOPT::USERAGENT:
  case CURLOPT::USERNAME:
  // case CURLOPT::USERPWD:
  case CURLOPT::ACCEPT_ENCODING: {
    auto res{kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_string(value.to_string()))};
    if (!res.has_value()) [[unlikely]] {
      print_error("Could not set an option", std::move(res.error()));
      return false;
    }
    set_errno(CURLE::OK);
    return true;
  }
  // "linked list" options
  // case CURLOPT::HTTP200ALIASES:
  case CURLOPT::HTTPHEADER:
  // case CURLOPT::POSTQUOTE:
  // case CURLOPT::PREQUOTE:
  // case CURLOPT::QUOTE:
  // case CURLOPT::MAIL_RCPT:
  case CURLOPT::RESOLVE: {
    if (unlikely(!value.is_array())) [[unlikely]] {
      bad_option_error("Value must be an array in function curl_setopt\n");
      return false;
    }
    const auto& arr = value.as_array();
    if (arr.empty()) [[unlikely]] {
      set_errno(CURLE::OK);
      return true;
    }
    array<string> arr_of_str{arr.size()};
    auto i{0};
    for (auto p : arr) {
      arr_of_str[i++] = f$strval(p.get_value());
    }
    auto res{kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_array_of_string(arr_of_str))};
    if (!res.has_value()) [[unlikely]] {
      print_error("Could not set an option", std::move(res.error()));
      return false;
    }
    set_errno(CURLE::OK);
    return true;
  }
  case CURLOPT::POSTFIELDS: {
    if (!value.is_array()) [[unlikely]] {
      auto res{kphp::web::set_transfer_prop(easy_id, CURLOPT::COPYPOSTFIELDS, kphp::web::PropertyValue::as_string(value.to_string()))};
      if (!res.has_value()) [[unlikely]] {
        print_error("Could not set an option", std::move(res.error()));
        return false;
      }
      set_errno(CURLE::OK);
      return true;
    }
    const auto& arr = value.as_array();
    if (arr.empty()) [[unlikely]] {
      set_errno(CURLE::OK);
      return true;
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
    auto res{kphp::web::set_transfer_prop(easy_id, CURLOPT::HTTPPOST, kphp::web::PropertyValue::as_array_of_string(arr_of_str))};
    if (!res.has_value()) [[unlikely]] {
      print_error("Could not set an option", std::move(res.error()));
      return false;
    }
    set_errno(CURLE::OK);
    return true;
  }
  // "enumeration" options
  case CURLOPT::PROXYTYPE: {
    auto proxy_type{value.to_int()};
    switch (proxy_type) {
    case CURLPROXY::HTTP:
    case CURLPROXY::HTTP_1_0:
    case CURLPROXY::SOCKS4:
    case CURLPROXY::SOCKS5:
    case CURLPROXY::SOCKS4A:
    case CURLPROXY::SOCKS5_HOSTNAME: {
      auto res{kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_long(proxy_type))};
      if (!res.has_value()) [[unlikely]] {
        print_error("Could not set an option", std::move(res.error()));
        return false;
      }
      set_errno(CURLE::OK);
      return true;
    }
    default:
      set_errno(CURLE::BAD_FUNCTION_ARGUMENT);
      return false;
    }
  }
  case CURLOPT::SSLVERSION: {
    auto ssl_version{value.to_int()};
    switch (ssl_version) {
    case CURL_SSLVERSION::DEFAULT:
    case CURL_SSLVERSION::TLSv1:
    // From https://www.php.net/manual/en/curl.constants.php
    // It is better to not set this option and leave the defaults.
    // As setting this to CURL_SSLVERSION_SSLv2 or CURL_SSLVERSION_SSLv3 is very dangerous, given the known vulnerabilities in SSLv2 and SSLv3.
    // case CURL_SSLVERSION::SSLv2:
    // case CURL_SSLVERSION::SSLv3:
    case CURL_SSLVERSION::TLSv1_0:
    case CURL_SSLVERSION::TLSv1_1:
    case CURL_SSLVERSION::TLSv1_2:
    case CURL_SSLVERSION::TLSv1_3: {
      auto res{kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_long(ssl_version))};
      if (!res.has_value()) [[unlikely]] {
        print_error("Could not set an option", std::move(res.error()));
        return false;
      }
      set_errno(CURLE::OK);
      return true;
    }
    default:
      set_errno(CURLE::BAD_FUNCTION_ARGUMENT);
      return false;
    }
  }
  case CURLOPT::IPRESOLVE: {
    auto ip_resolve{value.to_int()};
    switch (ip_resolve) {
    case CURL_IPRESOLVE::WHATEVER:
    case CURL_IPRESOLVE::V4:
    case CURL_IPRESOLVE::V6: {
      auto res{kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_long(ip_resolve))};
      if (!res.has_value()) [[unlikely]] {
        print_error("Could not set an option", std::move(res.error()));
        return false;
      }
      set_errno(CURLE::OK);
      return true;
    }
    default:
      set_errno(CURLE::BAD_FUNCTION_ARGUMENT);
      return false;
    }
  }
  case CURLOPT::NETRC: {
    auto netrc{value.to_int()};
    switch (netrc) {
    case CURL_NETRC::IGNORED:
    case CURL_NETRC::OPTIONAL:
    case CURL_NETRC::REQUIRED: {
      auto res{kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_long(netrc))};
      if (!res.has_value()) [[unlikely]] {
        print_error("Could not set an option", std::move(res.error()));
        return false;
      }
      set_errno(CURLE::OK);
      return true;
    }
    default:
      set_errno(CURLE::BAD_FUNCTION_ARGUMENT);
      return false;
    }
  }
  case CURLOPT::HTTP_VERSION: {
    auto http_version{value.to_int()};
    switch (http_version) {
    case CURL_HTTP_VERSION::NONE:
    case CURL_HTTP_VERSION::_1_0:
    case CURL_HTTP_VERSION::_1_1:
    case CURL_HTTP_VERSION::_2_0:
    case CURL_HTTP_VERSION::_2TLS:
    case CURL_HTTP_VERSION::_2_PRIOR_KNOWLEDGE: {
      auto res{kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_long(http_version))};
      if (!res.has_value()) [[unlikely]] {
        print_error("Could not set an option", std::move(res.error()));
        return false;
      }
      set_errno(CURLE::OK);
      return true;
    }
    default:
      set_errno(CURLE::BAD_FUNCTION_ARGUMENT);
      return false;
    }
  }
  // "auth" options
  case CURLOPT::HTTPAUTH:
  case CURLOPT::PROXYAUTH: {
    auto res{kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_long(value.to_int()))};
    if (!res.has_value()) [[unlikely]] {
      print_error("Could not set an option", std::move(res.error()));
      return false;
    }
    set_errno(CURLE::OK);
    return true;
  }
  case CURLOPT::PRIVATE: {
    easyctx_get_or_init(easy_id).private_data = value.to_string();
    set_errno(CURLE::OK);
    return true;
  }
  default:
    set_errno(CURLE::UNKNOWN_OPTION);
    return false;
  }
}

inline auto f$curl_setopt_array(curl_easy easy_id, const array<mixed>& options) noexcept -> bool {
  for (auto p : options) {
    if (!f$curl_setopt(easy_id, p.get_key().to_int(), p.get_value())) {
      return false;
    }
  }
  return true;
}

inline auto f$curl_exec(int64_t easy_id) noexcept -> kphp::coro::task<mixed> {
  auto res{co_await kphp::web::simple_transfer_perform(easy_id)};
  if (!res.has_value()) [[unlikely]] {
    set_errno(res.error().code, res.error().description);
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

inline auto f$curl_close(curl_easy easy_id) noexcept -> kphp::coro::task<void> {
  auto res{co_await kphp::web::simple_transfer_close(easy_id)};
  if (!res.has_value()) [[unlikely]] {
    set_errno(res.error().code, res.error().description);
    print_error("Could not close curl easy handle", std::move(res.error()));
  }
}
