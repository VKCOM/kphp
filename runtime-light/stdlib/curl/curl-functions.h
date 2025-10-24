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
#include "runtime-light/stdlib/web-transfer-lib/web.h"

namespace kphp::web::curl {

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
  set_errno(code, "");
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

} // namespace kphp::web::curl

inline auto f$curl_init(const string url = string("")) noexcept -> kphp::coro::task<kphp::web::curl::curl_easy> {
  auto open_res{co_await kphp::web::simple_transfer_open(kphp::web::TransferBackend::CURL)};
  if (!open_res.has_value()) [[unlikely]] {
    kphp::web::curl::set_errno(open_res.error().code, open_res.error().description);
    kphp::web::curl::print_error("Could not initialize a new curl easy handle", std::move(open_res.error()));
    co_return 0;
  }
  auto setopt_res{kphp::web::set_transfer_prop(open_res.value(), kphp::web::curl::CURLOPT::URL, kphp::web::PropertyValue::as_string(url))};
  if (!setopt_res.has_value()) [[unlikely]] {
    kphp::web::curl::set_errno(setopt_res.error().code, setopt_res.error().description);
    kphp::web::curl::print_error("Could not set URL for a new curl easy handle", std::move(setopt_res.error()));
    co_return 0;
  }
  co_return open_res.value();
}

inline auto f$curl_setopt(kphp::web::curl::curl_easy easy_id, int64_t option, const mixed& value) noexcept -> bool {
  if (option == kphp::web::curl::PHPCURL::INFO_HEADER_OUT) {
    auto res{kphp::web::set_transfer_prop(easy_id, kphp::web::curl::CURLOPT::VERBOSE, kphp::web::PropertyValue::as_long(value.to_int()))};
    if (!res.has_value()) [[unlikely]] {
      kphp::web::curl::print_error("Could not set an option", std::move(res.error()));
    }
    kphp::web::curl::set_errno(kphp::web::curl::CURLE::OK);
    return true;
  }

  if (option == kphp::web::curl::PHPCURL::OPT_RETURNTRANSFER) {
    auto& ctx{CurlInstanceState::get()};
    ctx.easyctx_get_or_init(easy_id).return_transfer = (value.to_int());
    kphp::web::curl::set_errno(kphp::web::curl::CURLE::OK);
    return true;
  }

  switch (option) {
  // "Long" options
  case kphp::web::curl::CURLOPT::ADDRESS_SCOPE:
  // case kphp::web::curl::CURLOPT::APPEND:
  case kphp::web::curl::CURLOPT::AUTOREFERER:
  case kphp::web::curl::CURLOPT::BUFFERSIZE:
  case kphp::web::curl::CURLOPT::CONNECT_ONLY:
  case kphp::web::curl::CURLOPT::CONNECTTIMEOUT:
  case kphp::web::curl::CURLOPT::CONNECTTIMEOUT_MS:
  case kphp::web::curl::CURLOPT::COOKIESESSION:
  // case kphp::web::curl::CURLOPT::CRLF:
  // case kphp::web::curl::CURLOPT::DIRLISTONLY:
  case kphp::web::curl::CURLOPT::DNS_CACHE_TIMEOUT:
  case kphp::web::curl::CURLOPT::FAILONERROR:
  case kphp::web::curl::CURLOPT::FILETIME:
  case kphp::web::curl::CURLOPT::FOLLOWLOCATION:
  case kphp::web::curl::CURLOPT::FORBID_REUSE:
  case kphp::web::curl::CURLOPT::FRESH_CONNECT:
  // case kphp::web::curl::CURLOPT::FTP_CREATE_MISSING_DIRS:
  // case kphp::web::curl::CURLOPT::FTP_RESPONSE_TIMEOUT:
  // case kphp::web::curl::CURLOPT::FTP_SKIP_PASV_IP:
  // case kphp::web::curl::CURLOPT::FTP_USE_EPRT:
  // case kphp::web::curl::CURLOPT::FTP_USE_EPSV:
  // case kphp::web::curl::CURLOPT::FTP_USE_PRET:
  case kphp::web::curl::CURLOPT::HEADER:
  case kphp::web::curl::CURLOPT::HTTP_CONTENT_DECODING:
  case kphp::web::curl::CURLOPT::HTTP_TRANSFER_DECODING:
  case kphp::web::curl::CURLOPT::HTTPGET:
  case kphp::web::curl::CURLOPT::HTTPPROXYTUNNEL:
  case kphp::web::curl::CURLOPT::IGNORE_CONTENT_LENGTH:
  // case kphp::web::curl::CURLOPT::INFILESIZE:
  case kphp::web::curl::CURLOPT::LOW_SPEED_LIMIT:
  case kphp::web::curl::CURLOPT::LOW_SPEED_TIME:
  case kphp::web::curl::CURLOPT::MAXCONNECTS:
  case kphp::web::curl::CURLOPT::MAXFILESIZE:
  case kphp::web::curl::CURLOPT::MAXREDIRS:
  // case kphp::web::curl::CURLOPT::NEW_DIRECTORY_PERMS:
  // case kphp::web::curl::CURLOPT::NEW_FILE_PERMS:
  case kphp::web::curl::CURLOPT::NOBODY:
  case kphp::web::curl::CURLOPT::PORT:
  case kphp::web::curl::CURLOPT::POST:
  // case kphp::web::curl::CURLOPT::PROXY_TRANSFER_MODE:
  case kphp::web::curl::CURLOPT::PROXYPORT:
  // case kphp::web::curl::CURLOPT::RESUME_FROM:
  // case kphp::web::curl::CURLOPT::SOCKS5_GSSAPI_NEC:
  case kphp::web::curl::CURLOPT::SSL_SESSIONID_CACHE:
  case kphp::web::curl::CURLOPT::SSL_VERIFYHOST:
  case kphp::web::curl::CURLOPT::SSL_VERIFYPEER:
  case kphp::web::curl::CURLOPT::SSLENGINE_DEFAULT:
  case kphp::web::curl::CURLOPT::TCP_NODELAY:
  // case kphp::web::curl::CURLOPT::TFTP_BLKSIZE:
  case kphp::web::curl::CURLOPT::TIMEOUT:
  case kphp::web::curl::CURLOPT::TIMEOUT_MS:
  // case kphp::web::curl::CURLOPT::TRANSFERTEXT:
  case kphp::web::curl::CURLOPT::UNRESTRICTED_AUTH:
  case kphp::web::curl::CURLOPT::UPLOAD:
  case kphp::web::curl::CURLOPT::VERBOSE:
  case kphp::web::curl::CURLOPT::WILDCARDMATCH:
  case kphp::web::curl::CURLOPT::PUT:
  case kphp::web::curl::CURLOPT::TCP_KEEPALIVE:
  case kphp::web::curl::CURLOPT::TCP_KEEPIDLE:
  case kphp::web::curl::CURLOPT::TCP_KEEPINTVL: {
    auto res{kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_long(value.to_int()))};
    if (!res.has_value()) [[unlikely]] {
      kphp::web::curl::print_error("Could not set an option", std::move(res.error()));
      return false;
    }
    kphp::web::curl::set_errno(kphp::web::curl::CURLE::OK);
    return true;
  }
  // "off_t" options
  case kphp::web::curl::CURLOPT::MAX_RECV_SPEED_LARGE:
  case kphp::web::curl::CURLOPT::MAX_SEND_SPEED_LARGE: {
    auto res{kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_long(value.to_int()))};
    if (!res.has_value()) [[unlikely]] {
      kphp::web::curl::print_error("Could not set an option", std::move(res.error()));
      return false;
    }
    kphp::web::curl::set_errno(kphp::web::curl::CURLE::OK);
    return true;
  }
  // "string" options
  case kphp::web::curl::CURLOPT::CAINFO:
  case kphp::web::curl::CURLOPT::CAPATH:
  case kphp::web::curl::CURLOPT::COOKIE:
  case kphp::web::curl::CURLOPT::COOKIEFILE:
  case kphp::web::curl::CURLOPT::COOKIEJAR:
  case kphp::web::curl::CURLOPT::COOKIELIST:
  case kphp::web::curl::CURLOPT::CRLFILE:
  case kphp::web::curl::CURLOPT::CUSTOMREQUEST:
  case kphp::web::curl::CURLOPT::EGDSOCKET:
  // case kphp::web::curl::CURLOPT::FTP_ACCOUNT:
  // case kphp::web::curl::CURLOPT::FTP_ALTERNATIVE_TO_USER:
  // case kphp::web::curl::CURLOPT::FTPPORT:
  case kphp::web::curl::CURLOPT::INTERFACE:
  case kphp::web::curl::CURLOPT::ISSUERCERT:
  // case kphp::web::curl::CURLOPT::KRBLEVEL:
  // case kphp::web::curl::CURLOPT::MAIL_FROM:
  // case kphp::web::curl::CURLOPT::NETRC_FILE:
  case kphp::web::curl::CURLOPT::NOPROXY:
  case kphp::web::curl::CURLOPT::PASSWORD:
  case kphp::web::curl::CURLOPT::PROXY:
  case kphp::web::curl::CURLOPT::PROXYPASSWORD:
  case kphp::web::curl::CURLOPT::PROXYUSERNAME:
  // case kphp::web::curl::CURLOPT::PROXYUSERPWD:
  case kphp::web::curl::CURLOPT::RANDOM_FILE:
  case kphp::web::curl::CURLOPT::RANGE:
  case kphp::web::curl::CURLOPT::REFERER:
  // case kphp::web::curl::CURLOPT::RTSP_SESSION_ID:
  // case kphp::web::curl::CURLOPT::RTSP_STREAM_URI:
  // case kphp::web::curl::CURLOPT::RTSP_TRANSPORT:
  // case kphp::web::curl::CURLOPT::SOCKS5_GSSAPI_SERVICE:
  // case kphp::web::curl::CURLOPT::SSH_HOST_PUBLIC_KEY_MD5:
  // case kphp::web::curl::CURLOPT::SSH_KNOWNHOSTS:
  // case kphp::web::curl::CURLOPT::SSH_PRIVATE_KEYFILE:
  // case kphp::web::curl::CURLOPT::SSH_PUBLIC_KEYFILE:
  case kphp::web::curl::CURLOPT::SSLCERT:
  case kphp::web::curl::CURLOPT::SSLCERTTYPE:
  case kphp::web::curl::CURLOPT::SSLENGINE:
  case kphp::web::curl::CURLOPT::SSLKEY:
  case kphp::web::curl::CURLOPT::SSLKEYTYPE:
  case kphp::web::curl::CURLOPT::SSL_CIPHER_LIST:
  case kphp::web::curl::CURLOPT::URL:
  case kphp::web::curl::CURLOPT::USERAGENT:
  case kphp::web::curl::CURLOPT::USERNAME:
  // case kphp::web::curl::CURLOPT::USERPWD:
  case kphp::web::curl::CURLOPT::ACCEPT_ENCODING: {
    auto res{kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_string(value.to_string()))};
    if (!res.has_value()) [[unlikely]] {
      kphp::web::curl::print_error("Could not set an option", std::move(res.error()));
      return false;
    }
    kphp::web::curl::set_errno(kphp::web::curl::CURLE::OK);
    return true;
  }
  // "linked list" options
  // case kphp::web::curl::CURLOPT::HTTP200ALIASES:
  case kphp::web::curl::CURLOPT::HTTPHEADER:
  // case kphp::web::curl::CURLOPT::POSTQUOTE:
  // case kphp::web::curl::CURLOPT::PREQUOTE:
  // case kphp::web::curl::CURLOPT::QUOTE:
  // case kphp::web::curl::CURLOPT::MAIL_RCPT:
  case kphp::web::curl::CURLOPT::RESOLVE: {
    if (unlikely(!value.is_array())) [[unlikely]] {
      kphp::web::curl::bad_option_error("Value must be an array in function curl_setopt\n");
      return false;
    }
    const auto& arr = value.as_array();
    if (arr.empty()) [[unlikely]] {
      kphp::web::curl::set_errno(kphp::web::curl::CURLE::OK);
      return true;
    }
    array<string> arr_of_str{arr.size()};
    auto i{0};
    for (auto p : arr) {
      arr_of_str[i++] = f$strval(p.get_value());
    }
    auto res{kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_array_of_string(arr_of_str))};
    if (!res.has_value()) [[unlikely]] {
      kphp::web::curl::print_error("Could not set an option", std::move(res.error()));
      return false;
    }
    kphp::web::curl::set_errno(kphp::web::curl::CURLE::OK);
    return true;
  }
  case kphp::web::curl::CURLOPT::POSTFIELDS: {
    if (!value.is_array()) [[unlikely]] {
      auto res{kphp::web::set_transfer_prop(easy_id, kphp::web::curl::CURLOPT::COPYPOSTFIELDS, kphp::web::PropertyValue::as_string(value.to_string()))};
      if (!res.has_value()) [[unlikely]] {
        kphp::web::curl::print_error("Could not set an option", std::move(res.error()));
        return false;
      }
      kphp::web::curl::set_errno(kphp::web::curl::CURLE::OK);
      return true;
    }
    const auto& arr = value.as_array();
    if (arr.empty()) [[unlikely]] {
      kphp::web::curl::set_errno(kphp::web::curl::CURLE::OK);
      return true;
    }
    array<string> arr_of_str{arr.size()};
    for (auto p : arr) {
      const string k = f$strval(p.get_key());
      const string v = f$strval(p.get_value());
      if (!v.empty() && v[0] == '@') [[unlikely]] {
        kphp::web::curl::bad_option_error("files posting is not supported in curl_setopt with CURLOPT_POSTFIELDS\n");
        return false;
      }
      arr_of_str[k] = v;
    }
    auto res{kphp::web::set_transfer_prop(easy_id, kphp::web::curl::CURLOPT::HTTPPOST, kphp::web::PropertyValue::as_array_of_string(arr_of_str))};
    if (!res.has_value()) [[unlikely]] {
      kphp::web::curl::print_error("Could not set an option", std::move(res.error()));
      return false;
    }
    kphp::web::curl::set_errno(kphp::web::curl::CURLE::OK);
    return true;
  }
  // "enumeration" options
  case kphp::web::curl::CURLOPT::PROXYTYPE: {
    auto proxy_type{value.to_int()};
    switch (proxy_type) {
    case kphp::web::curl::CURLPROXY::HTTP:
    case kphp::web::curl::CURLPROXY::HTTP_1_0:
    case kphp::web::curl::CURLPROXY::SOCKS4:
    case kphp::web::curl::CURLPROXY::SOCKS5:
    case kphp::web::curl::CURLPROXY::SOCKS4A:
    case kphp::web::curl::CURLPROXY::SOCKS5_HOSTNAME: {
      auto res{kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_long(proxy_type))};
      if (!res.has_value()) [[unlikely]] {
        kphp::web::curl::print_error("Could not set an option", std::move(res.error()));
        return false;
      }
      kphp::web::curl::set_errno(kphp::web::curl::CURLE::OK);
      return true;
    }
    default:
      kphp::web::curl::set_errno(kphp::web::curl::CURLE::BAD_FUNCTION_ARGUMENT);
      return false;
    }
  }
  case kphp::web::curl::CURLOPT::SSLVERSION: {
    auto ssl_version{value.to_int()};
    switch (ssl_version) {
    case kphp::web::curl::CURL_SSLVERSION::DEFAULT:
    case kphp::web::curl::CURL_SSLVERSION::TLSv1:
    // From https://www.php.net/manual/en/curl.constants.php
    // It is better to not set this option and leave the defaults.
    // As setting this to CURL_SSLVERSION_SSLv2 or CURL_SSLVERSION_SSLv3 is very dangerous, given the known vulnerabilities in SSLv2 and SSLv3.
    // case kphp::web::curl::CURL_SSLVERSION::SSLv2:
    // case kphp::web::curl::CURL_SSLVERSION::SSLv3:
    case kphp::web::curl::CURL_SSLVERSION::TLSv1_0:
    case kphp::web::curl::CURL_SSLVERSION::TLSv1_1:
    case kphp::web::curl::CURL_SSLVERSION::TLSv1_2:
    case kphp::web::curl::CURL_SSLVERSION::TLSv1_3: {
      auto res{kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_long(ssl_version))};
      if (!res.has_value()) [[unlikely]] {
        kphp::web::curl::print_error("Could not set an option", std::move(res.error()));
        return false;
      }
      kphp::web::curl::set_errno(kphp::web::curl::CURLE::OK);
      return true;
    }
    default:
      kphp::web::curl::set_errno(kphp::web::curl::CURLE::BAD_FUNCTION_ARGUMENT);
      return false;
    }
  }
  case kphp::web::curl::CURLOPT::IPRESOLVE: {
    auto ip_resolve{value.to_int()};
    switch (ip_resolve) {
    case kphp::web::curl::CURL_IPRESOLVE::WHATEVER:
    case kphp::web::curl::CURL_IPRESOLVE::V4:
    case kphp::web::curl::CURL_IPRESOLVE::V6: {
      auto res{kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_long(ip_resolve))};
      if (!res.has_value()) [[unlikely]] {
        kphp::web::curl::print_error("Could not set an option", std::move(res.error()));
        return false;
      }
      kphp::web::curl::set_errno(kphp::web::curl::CURLE::OK);
      return true;
    }
    default:
      kphp::web::curl::set_errno(kphp::web::curl::CURLE::BAD_FUNCTION_ARGUMENT);
      return false;
    }
  }
  case kphp::web::curl::CURLOPT::NETRC: {
    auto netrc{value.to_int()};
    switch (netrc) {
    case kphp::web::curl::CURL_NETRC::IGNORED:
    case kphp::web::curl::CURL_NETRC::OPTIONAL:
    case kphp::web::curl::CURL_NETRC::REQUIRED: {
      auto res{kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_long(netrc))};
      if (!res.has_value()) [[unlikely]] {
        kphp::web::curl::print_error("Could not set an option", std::move(res.error()));
        return false;
      }
      kphp::web::curl::set_errno(kphp::web::curl::CURLE::OK);
      return true;
    }
    default:
      kphp::web::curl::set_errno(kphp::web::curl::CURLE::BAD_FUNCTION_ARGUMENT);
      return false;
    }
  }
  case kphp::web::curl::CURLOPT::HTTP_VERSION: {
    auto http_version{value.to_int()};
    switch (http_version) {
    case kphp::web::curl::CURL_HTTP_VERSION::NONE:
    case kphp::web::curl::CURL_HTTP_VERSION::_1_0:
    case kphp::web::curl::CURL_HTTP_VERSION::_1_1:
    case kphp::web::curl::CURL_HTTP_VERSION::_2_0:
    case kphp::web::curl::CURL_HTTP_VERSION::_2TLS:
    case kphp::web::curl::CURL_HTTP_VERSION::_2_PRIOR_KNOWLEDGE: {
      auto res{kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_long(http_version))};
      if (!res.has_value()) [[unlikely]] {
        kphp::web::curl::print_error("Could not set an option", std::move(res.error()));
        return false;
      }
      kphp::web::curl::set_errno(kphp::web::curl::CURLE::OK);
      return true;
    }
    default:
      kphp::web::curl::set_errno(kphp::web::curl::CURLE::BAD_FUNCTION_ARGUMENT);
      return false;
    }
  }
  // "auth" options
  case kphp::web::curl::CURLOPT::HTTPAUTH:
  case kphp::web::curl::CURLOPT::PROXYAUTH: {
    auto res{kphp::web::set_transfer_prop(easy_id, option, kphp::web::PropertyValue::as_long(value.to_int()))};
    if (!res.has_value()) [[unlikely]] {
      kphp::web::curl::print_error("Could not set an option", std::move(res.error()));
      return false;
    }
    kphp::web::curl::set_errno(kphp::web::curl::CURLE::OK);
    return true;
  }
  case kphp::web::curl::CURLOPT::PRIVATE: {
    auto& ctx{CurlInstanceState::get()};
    ctx.easyctx_get_or_init(easy_id).private_data = value.to_string();
    kphp::web::curl::set_errno(kphp::web::curl::CURLE::OK);
    return true;
  }
  default:
    kphp::web::curl::set_errno(kphp::web::curl::CURLE::UNKNOWN_OPTION);
    return false;
  }
}

inline auto f$curl_setopt_array(kphp::web::curl::curl_easy easy_id, const array<mixed>& options) noexcept -> bool {
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
    kphp::web::curl::set_errno(res.error().code, res.error().description);
    kphp::web::curl::print_error("Could not exec curl easy handle", std::move(res.error()));
    co_return false;
  }
  auto& ctx{CurlInstanceState::get()};
  if (ctx.easy2ctx.contains(easy_id) && ctx.easy2ctx[easy_id].return_transfer) {
    co_return res.value().body;
  }
  print(res.value().body);
  co_return true;
}

inline auto f$curl_close(kphp::web::curl::curl_easy easy_id) noexcept -> kphp::coro::task<void> {
  auto res{co_await kphp::web::simple_transfer_close(easy_id)};
  if (!res.has_value()) [[unlikely]] {
    kphp::web::curl::set_errno(res.error().code, res.error().description);
    kphp::web::curl::print_error("Could not close curl easy handle", std::move(res.error()));
  }
}
