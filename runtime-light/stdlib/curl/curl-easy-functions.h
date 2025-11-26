// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <optional>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/curl/curl-context.h"
#include "runtime-light/stdlib/curl/curl-state.h"
#include "runtime-light/stdlib/curl/defs.h"
#include "runtime-light/stdlib/curl/details/diagnostics.h"
#include "runtime-light/stdlib/curl/details/normalize-timeout.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/fork/fork-functions.h"
#include "runtime-light/stdlib/output/print-functions.h"
#include "runtime-light/stdlib/web-transfer-lib/web-simple-transfer.h"

inline auto f$curl_init(string url = string{""}) noexcept -> kphp::coro::task<kphp::web::curl::easy_type> {
  auto open_res{co_await kphp::forks::id_managed(kphp::web::simple_transfer_open(kphp::web::transfer_backend::CURL))};
  if (!open_res.has_value()) [[unlikely]] {
    kphp::web::curl::print_error("could not initialize a new curl easy handle", std::move(open_res.error()));
    co_return 0;
  }
  const auto st{kphp::web::simple_transfer{*open_res}};
  auto setopt_res{
      kphp::web::set_transfer_property(st, static_cast<kphp::web::property_id>(kphp::web::curl::CURLOPT::URL), kphp::web::property_value::as_string(url))};
  if (!setopt_res.has_value()) [[unlikely]] {
    auto& easy_ctx{CurlInstanceState::get().easy_ctx.get_or_init(st.descriptor)};
    easy_ctx.set_errno(setopt_res.error().code, setopt_res.error().description);
    kphp::web::curl::print_error("could not set URL for a new curl easy handle", std::move(setopt_res.error()));
    co_return 0;
  }
  co_return st.descriptor;
}

inline auto f$curl_setopt(kphp::web::curl::easy_type easy_id, int64_t option, const mixed& value) noexcept -> bool {
  auto& easy_ctx{CurlInstanceState::get().easy_ctx.get_or_init(easy_id)};
  auto st{kphp::web::simple_transfer{.descriptor = easy_id}};

  if (option == static_cast<int64_t>(kphp::web::curl::CURLINFO::HEADER_OUT)) {
    auto res{kphp::web::set_transfer_property(st, static_cast<kphp::web::property_id>(kphp::web::curl::CURLOPT::VERBOSE),
                                              kphp::web::property_value::as_long(value.to_int()))};
    if (!res.has_value()) [[unlikely]] {
      kphp::web::curl::print_error("could not set an option", std::move(res.error()));
    }
    easy_ctx.set_errno(kphp::web::curl::CURLE::OK);
    return true;
  }

  if (option == static_cast<uint64_t>(kphp::web::curl::PHPCURL::RETURNTRANSFER)) {
    easy_ctx.return_transfer = (value.to_int());
    easy_ctx.set_errno(kphp::web::curl::CURLE::OK);
    return true;
  }

  switch (static_cast<kphp::web::curl::CURLOPT>(option)) {
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
    auto res{kphp::web::set_transfer_property(st, option, kphp::web::property_value::as_long(value.to_int()))};
    if (!res.has_value()) [[unlikely]] {
      kphp::web::curl::print_error("could not set an option", std::move(res.error()));
      return false;
    }
    easy_ctx.set_errno(kphp::web::curl::CURLE::OK);
    return true;
  }
  // "off_t" options
  case kphp::web::curl::CURLOPT::MAX_RECV_SPEED_LARGE:
  case kphp::web::curl::CURLOPT::MAX_SEND_SPEED_LARGE: {
    auto res{kphp::web::set_transfer_property(st, option, kphp::web::property_value::as_long(value.to_int()))};
    if (!res.has_value()) [[unlikely]] {
      kphp::web::curl::print_error("could not set an option", std::move(res.error()));
      return false;
    }
    easy_ctx.set_errno(kphp::web::curl::CURLE::OK);
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
    auto res{kphp::web::set_transfer_property(st, option, kphp::web::property_value::as_string(value.to_string()))};
    if (!res.has_value()) [[unlikely]] {
      kphp::web::curl::print_error("could not set an option", std::move(res.error()));
      return false;
    }
    easy_ctx.set_errno(kphp::web::curl::CURLE::OK);
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
    if (!value.is_array()) [[unlikely]] {
      easy_ctx.bad_option_error("value must be an array in function curl_setopt\n");
      return false;
    }
    auto& arr{value.as_array()};
    if (arr.empty()) [[unlikely]] {
      easy_ctx.set_errno(kphp::web::curl::CURLE::OK);
      return true;
    }
    array<string> arr_of_str{arr.size()};
    auto i{0};
    for (auto p : arr) {
      arr_of_str[i++] = f$strval(p.get_value());
    }
    auto res{kphp::web::set_transfer_property(st, option, kphp::web::property_value::as_array_of_string(arr_of_str))};
    if (!res.has_value()) [[unlikely]] {
      kphp::web::curl::print_error("could not set an option", std::move(res.error()));
      return false;
    }
    easy_ctx.set_errno(kphp::web::curl::CURLE::OK);
    return true;
  }
  case kphp::web::curl::CURLOPT::POSTFIELDS: {
    if (!value.is_array()) [[unlikely]] {
      auto res{kphp::web::set_transfer_property(st, static_cast<kphp::web::property_id>(kphp::web::curl::CURLOPT::COPYPOSTFIELDS),
                                                kphp::web::property_value::as_string(value.to_string()))};
      if (!res.has_value()) [[unlikely]] {
        kphp::web::curl::print_error("could not set an option", std::move(res.error()));
        return false;
      }
      easy_ctx.set_errno(kphp::web::curl::CURLE::OK);
      return true;
    }
    const auto& arr{value.as_array()};
    if (arr.empty()) [[unlikely]] {
      easy_ctx.set_errno(kphp::web::curl::CURLE::OK);
      return true;
    }
    array<string> arr_of_str{arr.size()};
    for (auto p : arr) {
      const string k = f$strval(p.get_key());
      const string v = f$strval(p.get_value());
      if (!v.empty() && v[0] == '@') [[unlikely]] {
        easy_ctx.bad_option_error("files posting is not supported in curl_setopt with CURLOPT_POSTFIELDS\n");
        return false;
      }
      arr_of_str[k] = v;
    }
    auto res{kphp::web::set_transfer_property(st, static_cast<kphp::web::property_id>(kphp::web::curl::CURLOPT::HTTPPOST),
                                              kphp::web::property_value::as_array_of_string(arr_of_str))};
    if (!res.has_value()) [[unlikely]] {
      kphp::web::curl::print_error("could not set an option", std::move(res.error()));
      return false;
    }
    easy_ctx.set_errno(kphp::web::curl::CURLE::OK);
    return true;
  }
  // "enumeration" options
  case kphp::web::curl::CURLOPT::PROXYTYPE: {
    auto proxy_type{static_cast<kphp::web::curl::CURLPROXY>(value.to_int())};
    switch (proxy_type) {
    case kphp::web::curl::CURLPROXY::HTTP:
    case kphp::web::curl::CURLPROXY::HTTP_1_0:
    case kphp::web::curl::CURLPROXY::SOCKS4:
    case kphp::web::curl::CURLPROXY::SOCKS5:
    case kphp::web::curl::CURLPROXY::SOCKS4A:
    case kphp::web::curl::CURLPROXY::SOCKS5_HOSTNAME: {
      auto res{kphp::web::set_transfer_property(st, option, kphp::web::property_value::as_long(static_cast<int64_t>(proxy_type)))};
      if (!res.has_value()) [[unlikely]] {
        kphp::web::curl::print_error("could not set an option", std::move(res.error()));
        return false;
      }
      easy_ctx.set_errno(kphp::web::curl::CURLE::OK);
      return true;
    }
    default:
      easy_ctx.set_errno(kphp::web::curl::CURLE::BAD_FUNCTION_ARGUMENT, "a libcurl function was given an incorrect PROXYTYPE kind");
      return false;
    }
  }
  case kphp::web::curl::CURLOPT::SSLVERSION: {
    auto ssl_version{static_cast<kphp::web::curl::CURL_SSLVERSION>(value.to_int())};
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
      auto res{kphp::web::set_transfer_property(st, option, kphp::web::property_value::as_long(static_cast<int64_t>(ssl_version)))};
      if (!res.has_value()) [[unlikely]] {
        kphp::web::curl::print_error("could not set an option", std::move(res.error()));
        return false;
      }
      easy_ctx.set_errno(kphp::web::curl::CURLE::OK);
      return true;
    }
    default:
      easy_ctx.set_errno(kphp::web::curl::CURLE::BAD_FUNCTION_ARGUMENT, "a libcurl function was given an incorrect SSLVERSION kind");
      return false;
    }
  }
  case kphp::web::curl::CURLOPT::IPRESOLVE: {
    auto ip_resolve{static_cast<kphp::web::curl::CURL_IPRESOLVE>(value.to_int())};
    switch (ip_resolve) {
    case kphp::web::curl::CURL_IPRESOLVE::WHATEVER:
    case kphp::web::curl::CURL_IPRESOLVE::V4:
    case kphp::web::curl::CURL_IPRESOLVE::V6: {
      auto res{kphp::web::set_transfer_property(st, option, kphp::web::property_value::as_long(static_cast<int64_t>(ip_resolve)))};
      if (!res.has_value()) [[unlikely]] {
        kphp::web::curl::print_error("could not set an option", std::move(res.error()));
        return false;
      }
      easy_ctx.set_errno(kphp::web::curl::CURLE::OK);
      return true;
    }
    default:
      easy_ctx.set_errno(kphp::web::curl::CURLE::BAD_FUNCTION_ARGUMENT, "a libcurl function was given an incorrect IPRESOLVE kind");
      return false;
    }
  }
  case kphp::web::curl::CURLOPT::NETRC: {
    auto netrc{static_cast<kphp::web::curl::CURL_NETRC>(value.to_int())};
    switch (netrc) {
    case kphp::web::curl::CURL_NETRC::IGNORED:
    case kphp::web::curl::CURL_NETRC::OPTIONAL:
    case kphp::web::curl::CURL_NETRC::REQUIRED: {
      auto res{kphp::web::set_transfer_property(st, option, kphp::web::property_value::as_long(static_cast<int64_t>(netrc)))};
      if (!res.has_value()) [[unlikely]] {
        kphp::web::curl::print_error("could not set an option", std::move(res.error()));
        return false;
      }
      easy_ctx.set_errno(kphp::web::curl::CURLE::OK);
      return true;
    }
    default:
      easy_ctx.set_errno(kphp::web::curl::CURLE::BAD_FUNCTION_ARGUMENT, "a libcurl function was given an incorrect NETRC kind");
      return false;
    }
  }
  case kphp::web::curl::CURLOPT::HTTP_VERSION: {
    auto http_version{static_cast<kphp::web::curl::CURL_HTTP_VERSION>(value.to_int())};
    switch (http_version) {
    case kphp::web::curl::CURL_HTTP_VERSION::NONE:
    case kphp::web::curl::CURL_HTTP_VERSION::_1_0:
    case kphp::web::curl::CURL_HTTP_VERSION::_1_1:
    case kphp::web::curl::CURL_HTTP_VERSION::_2_0:
    case kphp::web::curl::CURL_HTTP_VERSION::_2TLS:
    case kphp::web::curl::CURL_HTTP_VERSION::_2_PRIOR_KNOWLEDGE: {
      auto res{kphp::web::set_transfer_property(st, option, kphp::web::property_value::as_long(static_cast<int64_t>(http_version)))};
      if (!res.has_value()) [[unlikely]] {
        kphp::web::curl::print_error("could not set an option", std::move(res.error()));
        return false;
      }
      easy_ctx.set_errno(kphp::web::curl::CURLE::OK);
      return true;
    }
    default:
      easy_ctx.set_errno(kphp::web::curl::CURLE::BAD_FUNCTION_ARGUMENT, "a libcurl function was given an incorrect HTTP_VERSION kind");
      return false;
    }
  }
  // "auth" options
  case kphp::web::curl::CURLOPT::HTTPAUTH:
  case kphp::web::curl::CURLOPT::PROXYAUTH: {
    auto res{kphp::web::set_transfer_property(st, option, kphp::web::property_value::as_long(value.to_int()))};
    if (!res.has_value()) [[unlikely]] {
      kphp::web::curl::print_error("could not set an option", std::move(res.error()));
      return false;
    }
    easy_ctx.set_errno(kphp::web::curl::CURLE::OK);
    return true;
  }
  case kphp::web::curl::CURLOPT::PRIVATE: {
    easy_ctx.private_data = value.to_string();
    easy_ctx.set_errno(kphp::web::curl::CURLE::OK);
    return true;
  }
  default:
    easy_ctx.set_errno(kphp::web::curl::CURLE::UNKNOWN_OPTION);
    return false;
  }
}

inline auto f$curl_setopt_array(kphp::web::curl::easy_type easy_id, const array<mixed>& options) noexcept -> bool {
  for (auto p : options) {
    if (!f$curl_setopt(easy_id, p.get_key().to_int(), p.get_value())) {
      return false;
    }
  }
  return true;
}

inline auto f$curl_exec(kphp::web::curl::easy_type easy_id) noexcept -> kphp::coro::task<mixed> {
  auto res{co_await kphp::forks::id_managed(kphp::web::simple_transfer_perform(kphp::web::simple_transfer{easy_id}))};
  auto& easy_ctx{CurlInstanceState::get().easy_ctx.get_or_init(easy_id)};
  easy_ctx.has_been_executed = true;
  if (!res.has_value()) [[unlikely]] {
    easy_ctx.set_errno(res.error().code, res.error().description);
    kphp::web::curl::print_error("could not execute curl easy handle", std::move(res.error()));
    co_return false;
  }
  if (easy_ctx.return_transfer) {
    co_return std::move((*res).body);
  }
  print(std::move((*res).body));
  co_return true;
}

inline auto f$curl_close(kphp::web::curl::easy_type easy_id) noexcept -> kphp::coro::task<void> {
  auto& easy_ctx{CurlInstanceState::get().easy_ctx.get_or_init(easy_id)};
  auto res{co_await kphp::forks::id_managed(kphp::web::simple_transfer_close(kphp::web::simple_transfer{easy_id}))};
  if (!res.has_value()) [[unlikely]] {
    easy_ctx.set_errno(res.error().code, res.error().description);
    kphp::web::curl::print_error("could not close curl easy handle", std::move(res.error()));
  }
}

inline auto f$curl_reset(kphp::web::curl::easy_type easy_id) noexcept -> kphp::coro::task<void> {
  auto& easy_ctx{CurlInstanceState::get().easy_ctx.get_or_init(easy_id)};
  auto res{co_await kphp::forks::id_managed(kphp::web::simple_transfer_reset(kphp::web::simple_transfer{easy_id}))};
  if (!res.has_value()) [[unlikely]] {
    easy_ctx.set_errno(res.error().code, res.error().description);
    kphp::web::curl::print_error("could not reset curl easy handle", std::move(res.error()));
    co_return;
  }
  easy_ctx.return_transfer = false;
  easy_ctx.private_data = std::nullopt;
}

inline auto f$curl_exec_concurrently(kphp::web::curl::easy_type easy_id, double timeout_sec = 1.0) noexcept -> kphp::coro::task<Optional<string>> {
  auto& easy_ctx{CurlInstanceState::get().easy_ctx.get_or_init(easy_id)};
  auto sched_res{co_await kphp::coro::io_scheduler::get().schedule(
      kphp::forks::id_managed(kphp::web::simple_transfer_perform(kphp::web::simple_transfer{easy_id})),
      kphp::web::curl::details::normalize_timeout(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{timeout_sec})))};
  if (!sched_res.has_value()) [[unlikely]] {
    kphp::web::curl::print_error(
        "could not execute curl easy handle concurrently",
        kphp::web::error{.code = kphp::web::WEB_INTERNAL_ERROR_CODE, .description = string{"concurrent transfer has been interrupted due to timeout"}});
    co_return false;
  }
  auto& perform_res{*sched_res};
  easy_ctx.has_been_executed = true;
  if (!perform_res.has_value()) [[unlikely]] {
    easy_ctx.set_errno(perform_res.error().code, perform_res.error().description);
    kphp::web::curl::print_error("could not execute curl easy handle concurrently", std::move(perform_res.error()));
    co_return false;
  }
  co_return std::move((*perform_res).body);
}

inline auto f$curl_error(kphp::web::curl::easy_type easy_id) noexcept -> string {
  auto& easy_ctx{CurlInstanceState::get().easy_ctx.get_or_init(easy_id)};
  if (easy_ctx.error_code != static_cast<int64_t>(kphp::web::curl::CURLE::OK)) [[likely]] {
    const auto* const desc_data{reinterpret_cast<const char*>(easy_ctx.error_description.data())};
    const auto desc_size{static_cast<string::size_type>(easy_ctx.error_description.size())};
    return string{desc_data, desc_size};
  }
  return {};
}

inline auto f$curl_errno(kphp::web::curl::easy_type easy_id) noexcept -> int64_t {
  auto& easy_ctx{CurlInstanceState::get().easy_ctx.get_or_init(easy_id)};
  return easy_ctx.error_code;
}

inline auto f$curl_getinfo(kphp::web::curl::easy_type easy_id, int64_t option = 0) noexcept -> kphp::coro::task<mixed> {
  auto& easy_ctx{CurlInstanceState::get().easy_ctx.get_or_init(easy_id)};
  switch (static_cast<kphp::web::curl::CURLINFO>(option)) {
  case kphp::web::curl::CURLINFO::NONE: {
    auto res{co_await kphp::forks::id_managed(kphp::web::get_transfer_properties(kphp::web::simple_transfer{easy_id}, std::nullopt))};
    if (!res.has_value()) [[unlikely]] {
      kphp::web::curl::print_error("could not get all info options of easy handle", std::move(res.error()));
      co_return false;
    }

    const auto& info{*res};
    array<mixed> result{array_size{22, false}};
    const auto set_value{[&result, &info](const string& key, const kphp::web::curl::CURLINFO what) {
      const auto& v{info.find(static_cast<kphp::web::property_id>(what))};
      kphp::log::assertion(v != info.end());
      auto value{v->second.to_mixed()};
      if (!equals(value, false)) {
        result.set_value(std::move(key), std::move(value));
      }
    }};

    if (!easy_ctx.has_been_executed) {
      const auto url_opt_id{static_cast<kphp::web::property_id>(kphp::web::curl::CURLOPT::URL)};
      const auto url{co_await kphp::forks::id_managed(kphp::web::get_transfer_properties(kphp::web::simple_transfer{easy_id}, url_opt_id))};
      if (url.has_value()) {
        const auto& v{(*url).find(url_opt_id)};
        kphp::log::assertion(v != (*url).end());
        result.set_value(string{"url"}, v->second.to_mixed());
      } else {
        result.set_value(string{"url"}, string{});
      }
    } else {
      set_value(string{"url"}, kphp::web::curl::CURLINFO::EFFECTIVE_URL);
    }
    set_value(string{"content_type"}, kphp::web::curl::CURLINFO::CONTENT_TYPE);
    set_value(string{"http_code"}, kphp::web::curl::CURLINFO::RESPONSE_CODE);
    set_value(string{"header_size"}, kphp::web::curl::CURLINFO::HEADER_SIZE);
    set_value(string{"request_size"}, kphp::web::curl::CURLINFO::REQUEST_SIZE);
    set_value(string{"filetime"}, kphp::web::curl::CURLINFO::FILETIME);
    set_value(string{"redirect_count"}, kphp::web::curl::CURLINFO::REDIRECT_COUNT);
    set_value(string{"total_time"}, kphp::web::curl::CURLINFO::TOTAL_TIME);
    set_value(string{"namelookup_time"}, kphp::web::curl::CURLINFO::NAMELOOKUP_TIME);
    set_value(string{"connect_time"}, kphp::web::curl::CURLINFO::CONNECT_TIME);
    set_value(string{"pretransfer_time"}, kphp::web::curl::CURLINFO::PRETRANSFER_TIME);
    set_value(string{"size_upload"}, kphp::web::curl::CURLINFO::SIZE_UPLOAD);
    set_value(string{"size_download"}, kphp::web::curl::CURLINFO::SIZE_DOWNLOAD);
    set_value(string{"download_content_length"}, kphp::web::curl::CURLINFO::CONTENT_LENGTH_DOWNLOAD);
    set_value(string{"starttransfer_time"}, kphp::web::curl::CURLINFO::STARTTRANSFER_TIME);
    set_value(string{"redirect_time"}, kphp::web::curl::CURLINFO::REDIRECT_TIME);
    set_value(string{"redirect_url"}, kphp::web::curl::CURLINFO::REDIRECT_URL);
    set_value(string{"primary_ip"}, kphp::web::curl::CURLINFO::PRIMARY_IP);
    set_value(string{"primary_port"}, kphp::web::curl::CURLINFO::PRIMARY_PORT);
    set_value(string{"local_ip"}, kphp::web::curl::CURLINFO::LOCAL_IP);
    set_value(string{"local_port"}, kphp::web::curl::CURLINFO::LOCAL_PORT);
    set_value(string{"request_header"}, kphp::web::curl::CURLINFO::HEADER_OUT);
    co_return result;
  }
  case kphp::web::curl::CURLINFO::PRIVATE: {
    const auto data{easy_ctx.private_data};
    if (data.has_value()) {
      co_return *data;
    }
    co_return false;
  }
  case kphp::web::curl::CURLINFO::EFFECTIVE_URL:
    if (!easy_ctx.has_been_executed) {
      const auto url_opt_id{static_cast<kphp::web::property_id>(kphp::web::curl::CURLOPT::URL)};
      const auto url{co_await kphp::forks::id_managed(kphp::web::get_transfer_properties(kphp::web::simple_transfer{easy_id}, url_opt_id))};
      if (url.has_value()) {
        co_return (*url).find(url_opt_id)->second.to_mixed();
      }
      co_return string{};
    }
  case kphp::web::curl::CURLINFO::CONTENT_TYPE:
  case kphp::web::curl::CURLINFO::RESPONSE_CODE:
  case kphp::web::curl::CURLINFO::HEADER_SIZE:
  case kphp::web::curl::CURLINFO::REQUEST_SIZE:
  case kphp::web::curl::CURLINFO::FILETIME:
  case kphp::web::curl::CURLINFO::REDIRECT_COUNT:
  case kphp::web::curl::CURLINFO::TOTAL_TIME:
  case kphp::web::curl::CURLINFO::NAMELOOKUP_TIME:
  case kphp::web::curl::CURLINFO::CONNECT_TIME:
  case kphp::web::curl::CURLINFO::PRETRANSFER_TIME:
  case kphp::web::curl::CURLINFO::SIZE_UPLOAD:
  case kphp::web::curl::CURLINFO::SIZE_DOWNLOAD:
  case kphp::web::curl::CURLINFO::CONTENT_LENGTH_DOWNLOAD:
  case kphp::web::curl::CURLINFO::STARTTRANSFER_TIME:
  case kphp::web::curl::CURLINFO::REDIRECT_TIME:
  case kphp::web::curl::CURLINFO::REDIRECT_URL:
  case kphp::web::curl::CURLINFO::PRIMARY_IP:
  case kphp::web::curl::CURLINFO::PRIMARY_PORT:
  case kphp::web::curl::CURLINFO::LOCAL_IP:
  case kphp::web::curl::CURLINFO::LOCAL_PORT:
  case kphp::web::curl::CURLINFO::HTTP_CONNECTCODE:
  case kphp::web::curl::CURLINFO::OS_ERRNO:
  case kphp::web::curl::CURLINFO::APPCONNECT_TIME:
  case kphp::web::curl::CURLINFO::CONDITION_UNMET:
  case kphp::web::curl::CURLINFO::HEADER_OUT: {
    auto res{co_await kphp::forks::id_managed(kphp::web::get_transfer_properties(kphp::web::simple_transfer{easy_id}, option))};
    if (!res.has_value()) [[unlikely]] {
      kphp::web::curl::print_error("could not get a specific info of easy handle", std::move(res.error()));
      co_return 0;
    }
    const auto& v{(*res).find(option)};
    kphp::log::assertion(v != (*res).end());
    co_return v->second.to_mixed();
  }
  default:
    co_return false;
  }
}
