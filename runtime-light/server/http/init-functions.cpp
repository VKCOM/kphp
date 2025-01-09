// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/server/http/init-functions.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <optional>
#include <string_view>
#include <system_error>
#include <utility>

#include "absl/strings/match.h"

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-common/stdlib/array/array-functions.h"
#include "runtime-common/stdlib/server/url-functions.h"
#include "runtime-light/core/globals/php-script-globals.h"
#include "runtime-light/server/http/http-server-state.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/stdlib/server/http-functions.h"
#include "runtime-light/stdlib/zlib/zlib-functions.h"
#include "runtime-light/streams/streams.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"

namespace {

constexpr std::string_view HTTP = "HTTP";
constexpr std::string_view HTTPS = "HTTPS";
constexpr std::string_view HTTP_SCHEME = "http";
constexpr std::string_view HTTPS_SCHEME = "https";
constexpr std::string_view HTTP_HEADER_PREFIX = "HTTP_";
constexpr std::string_view HTTP_X_REAL_HOST = "HTTP_X_REAL_HOST";
constexpr std::string_view HTTP_X_REAL_SCHEME = "HTTP_X_REAL_SCHEME";
constexpr std::string_view HTTP_X_REAL_REQUEST = "HTTP_X_REAL_REQUEST";

constexpr std::string_view SCHEME_SUFFIX = "://";
constexpr std::string_view GATEWAY_INTERFACE_VALUE = "CGI/1.1";

constexpr std::string_view AUTHORIZATION_BASIC = "basic";
constexpr std::string_view CONNECTION_CLOSE = "close";
constexpr std::string_view CONNECTION_KEEP_ALIVE = "keep-alive";
constexpr std::string_view ENCODING_GZIP = "gzip";
constexpr std::string_view ENCODING_DEFLATE = "deflate";
constexpr std::string_view CONTENT_TYPE_TEXT_WIN1251 = "text/html; charset=windows-1251";
constexpr std::string_view CONTENT_TYPE_APP_FORM_URLENCODED = "application/x-www-form-urlencoded";
constexpr std::string_view CONTENT_TYPE_MULTIPART_FORM_DATA = "multipart/form-data";

constexpr std::string_view GET_METHOD = "GET";
constexpr std::string_view POST_METHOD = "POST";
constexpr std::string_view HEAD_METHOD = "HEAD";

string get_server_protocol(tl::HttpVersion http_version, const std::optional<std::string_view> &opt_scheme) noexcept {
  std::string_view protocol_name{HTTP};
  const auto protocol_version{http_version.string_view()};
  if (opt_scheme.has_value()) {
    const std::string_view scheme{*opt_scheme};
    if (scheme == HTTPS_SCHEME) {
      protocol_name = HTTPS;
    } else if (scheme != HTTP_SCHEME) [[unlikely]] {
      php_error("unexpected http scheme: %s", scheme.data());
    }
  }
  string protocol{};
  protocol.reserve_at_least(protocol_name.size() + protocol_version.size() + 1); // +1 for '/'
  protocol.append(protocol_name.data(), protocol_name.size());
  protocol.append(1, '/');
  protocol.append(protocol_version.data(), protocol_version.size());
  return protocol;
}

void process_cookie_header(std::string_view header, PhpScriptBuiltInSuperGlobals &superglobals) noexcept {
  // *** be careful here ***
  auto *cookie_start{const_cast<char *>(header.data())};
  auto *cookie_list_end{const_cast<char *>(std::next(header.data(), header.size()))};
  do {
    auto *cookie_end{std::find(cookie_start, cookie_list_end, ';')};
    char *cookie_domain_end{std::find(cookie_start, cookie_end, '=')};
    if (cookie_domain_end != cookie_end) [[likely]] {
      char *cookie_value_start{std::next(cookie_domain_end)};
      const auto cookie_domain_size{static_cast<string::size_type>(std::distance(cookie_start, cookie_domain_end))};
      const auto cookie_value_size{static_cast<string::size_type>(std::distance(cookie_value_start, cookie_end))};
      string cookie_domain{cookie_start, cookie_domain_size};
      string cookie_value{cookie_value_start, cookie_value_size};
      parse_str_set_value(superglobals.v$_COOKIE, cookie_domain, f$urldecode(cookie_value));
    }
    // skip ';' if possible
    cookie_start = cookie_end == cookie_list_end ? cookie_list_end : std::next(cookie_end);
    // skip whitespaces
    while (cookie_start != cookie_list_end && *cookie_start == ' ') {
      cookie_start = std::next(cookie_start);
    }
  } while (cookie_start != cookie_list_end);
}

// RFC link: https://tools.ietf.org/html/rfc2617#section-2
// Header example:
//  Authorization: Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ==
void process_authorization_header(const string &header, PhpScriptBuiltInSuperGlobals &superglobals) noexcept {
  array<string> header_parts{explode(' ', header)};
  if (header_parts.count() != 2) [[unlikely]] {
    return;
  }

  const auto &auth_scheme{header_parts[0]};
  const auto &auth_credentials{header_parts[1]};
  if (std::string_view{auth_scheme.c_str(), auth_scheme.size()} != AUTHORIZATION_BASIC) [[unlikely]] {
    return;
  }

  const auto decoded_login_pass{f$base64_decode(auth_credentials, true)};
  if (!decoded_login_pass.has_value()) [[unlikely]] {
    return;
  }

  array<string> auth_data{explode(':', decoded_login_pass.val())};
  if (auth_data.count() != 2) [[unlikely]] {
    return;
  }

  using namespace PhpServerSuperGlobalIndices;
  superglobals.v$_SERVER.set_value(string{PHP_AUTH_USER.data(), PHP_AUTH_USER.size()}, auth_data[0]);
  superglobals.v$_SERVER.set_value(string{PHP_AUTH_PW.data(), PHP_AUTH_PW.size()}, auth_data[1]);
  superglobals.v$_SERVER.set_value(string{AUTH_TYPE.data(), AUTH_TYPE.size()}, auth_scheme);
}

// returns content type
std::string_view process_headers(tl::K2InvokeHttp &invoke_http, PhpScriptBuiltInSuperGlobals &superglobals) noexcept {
  auto &server{superglobals.v$_SERVER};
  auto &http_server_instance_st{HttpServerInstanceState::get()};

  std::string_view content_type{CONTENT_TYPE_APP_FORM_URLENCODED};
  // platform provides headers that are already in lowercase
  for (auto &[_, h_name, h_value] : invoke_http.headers) {

    using namespace PhpServerSuperGlobalIndices;
    if (h_name == kphp::http::headers::ACCEPT_ENCODING) {
      if (absl::StrContains(h_value, ENCODING_GZIP)) {
        http_server_instance_st.encoding |= HttpServerInstanceState::ENCODING_GZIP;
      }
      if (absl::StrContains(h_value, ENCODING_DEFLATE)) {
        http_server_instance_st.encoding |= HttpServerInstanceState::ENCODING_DEFLATE;
      }
    } else if (h_name == kphp::http::headers::CONNECTION) {
      if (h_value == CONNECTION_KEEP_ALIVE) [[likely]] {
        http_server_instance_st.connection_kind = kphp::http::connection_kind::keep_alive;
      } else if (h_value == CONNECTION_CLOSE) [[likely]] {
        http_server_instance_st.connection_kind = kphp::http::connection_kind::close;
      } else {
        php_error("unexpected connection header: %s", h_value.data());
      }
    } else if (h_name == kphp::http::headers::COOKIE) {
      process_cookie_header(h_value, superglobals);
    } else if (h_name == kphp::http::headers::HOST) {
      server.set_value(string{SERVER_NAME.data(), SERVER_NAME.size()}, string{h_value.data(), static_cast<string::size_type>(h_value.size())});
    } else if (h_name == kphp::http::headers::AUTHORIZATION) {
      process_authorization_header({h_value.data(), static_cast<string::size_type>(h_value.size())}, superglobals);
    } else if (h_name == kphp::http::headers::CONTENT_TYPE) {
      content_type = h_value;
      continue;
    } else if (h_name == kphp::http::headers::CONTENT_LENGTH) {
      int32_t content_length{};
      const auto [_, ec]{std::from_chars(h_value.data(), std::next(h_value.data(), h_value.size()), content_length)};
      if (ec != std::errc{} || content_length != invoke_http.body.size()) [[unlikely]] {
        php_error("content-length expected to be %d, but it's %zu", content_length, invoke_http.body.size());
      }
      continue;
    }

    // add header entries
    string key{};
    key.reserve_at_least(HTTP_HEADER_PREFIX.size() + h_name.size());
    key.append(HTTP_HEADER_PREFIX.data());
    key.append(h_name.data(), h_name.size());
    // to uppercase inplace
    for (int64_t i = HTTP_HEADER_PREFIX.size(); i < key.size(); ++i) {
      key[i] = key[i] != '-' ? std::toupper(key[i]) : '_';
    }
    server.set_value(key, string{h_value.data(), static_cast<string::size_type>(h_value.size())});
  }

  return content_type;
}

} // namespace

namespace kphp {

namespace http {

void init_server(tl::K2InvokeHttp &&invoke_http) noexcept {
  auto &superglobals{InstanceState::get().php_script_mutable_globals_singleton.get_superglobals()};
  auto &server{superglobals.v$_SERVER};
  auto &http_server_instance_st{HttpServerInstanceState::get()};

  // determine HTTP method
  if (invoke_http.method == GET_METHOD) {
    http_server_instance_st.http_method = method::get;
  } else if (invoke_http.method == POST_METHOD) {
    http_server_instance_st.http_method = method::post;
  } else if (invoke_http.method == HEAD_METHOD) [[likely]] {
    http_server_instance_st.http_method = method::head;
  } else {
    http_server_instance_st.http_method = method::other;
  }

  const string uri_path{invoke_http.uri.path.data(), static_cast<string::size_type>(invoke_http.uri.path.size())};
  const string uri_query{invoke_http.uri.opt_query.has_value()
                           ? string{(*invoke_http.uri.opt_query).data(), static_cast<string::size_type>((*invoke_http.uri.opt_query).size())}
                           : string{}};

  using namespace PhpServerSuperGlobalIndices;
  server.set_value(string{PHP_SELF.data(), PHP_SELF.size()}, uri_path);
  server.set_value(string{SCRIPT_NAME.data(), SCRIPT_NAME.size()}, uri_path);
  server.set_value(string{SCRIPT_URL.data(), SCRIPT_URL.size()}, uri_path);

  server.set_value(string{SERVER_ADDR.data(), SERVER_ADDR.size()},
                   string{invoke_http.connection.server_addr.data(), static_cast<string::size_type>(invoke_http.connection.server_addr.size())});
  server.set_value(string{SERVER_PORT.data(), SERVER_PORT.size()}, static_cast<int64_t>(invoke_http.connection.server_port));
  server.set_value(string{SERVER_PROTOCOL.data(), SERVER_PROTOCOL.size()}, get_server_protocol(invoke_http.version, invoke_http.uri.opt_scheme));
  server.set_value(string{REMOTE_ADDR.data(), REMOTE_ADDR.size()},
                   string{invoke_http.connection.remote_addr.data(), static_cast<string::size_type>(invoke_http.connection.remote_addr.size())});
  server.set_value(string{REMOTE_PORT.data(), REMOTE_PORT.size()}, static_cast<int64_t>(invoke_http.connection.remote_port));

  server.set_value(string{REQUEST_METHOD.data(), REQUEST_METHOD.size()},
                   string{invoke_http.method.data(), static_cast<string::size_type>(invoke_http.method.size())});
  server.set_value(string{GATEWAY_INTERFACE.data(), GATEWAY_INTERFACE.size()}, string{GATEWAY_INTERFACE_VALUE.data(), GATEWAY_INTERFACE_VALUE.size()});

  if (invoke_http.uri.opt_query.has_value()) {
    f$parse_str(uri_query, superglobals.v$_GET);
    server.set_value(string{QUERY_STRING.data(), QUERY_STRING.size()}, uri_query);

    string uri_string{};
    uri_string.reserve_at_least(uri_path.size() + uri_query.size() + 1); // +1 for '?'
    uri_string.append(uri_path);
    uri_string.append(1, '?');
    uri_string.append(uri_query);
    server.set_value(string{REQUEST_URI.data(), REQUEST_URI.size()}, uri_string);
  } else {
    server.set_value(string{REQUEST_URI.data(), REQUEST_URI.size()}, uri_path);
  }

  if (invoke_http.uri.opt_scheme.value_or(std::string_view{}) == HTTPS_SCHEME) {
    server.set_value(string{HTTPS.data(), HTTPS.size()}, true);
  }

  const auto content_type{process_headers(invoke_http, superglobals)};

  string real_scheme_header{HTTP_X_REAL_SCHEME.data(), HTTP_X_REAL_SCHEME.size()};
  string real_host_header{HTTP_X_REAL_HOST.data(), HTTP_X_REAL_HOST.size()};
  string real_request_header{HTTP_X_REAL_REQUEST.data(), HTTP_X_REAL_REQUEST.size()};
  if (server.isset(real_scheme_header) && server.isset(real_host_header) && server.isset(real_request_header)) {
    string real_scheme{server.get_value(real_request_header).to_string()};
    string real_host{server.get_value(real_host_header).to_string()};
    string real_request{server.get_value(real_request_header).to_string()};

    string script_uri{};
    script_uri.reserve_at_least(real_scheme.size() + SCHEME_SUFFIX.size() + real_host.size() + real_request.size());
    script_uri.append(real_scheme);
    script_uri.append(SCHEME_SUFFIX.data(), SCHEME_SUFFIX.size());
    script_uri.append(real_host);
    script_uri.append(real_request);
    server.set_value(string{SCRIPT_URI.data(), SCRIPT_URI.size()}, script_uri);
  }

  if (http_server_instance_st.http_method == method::get) {
    server.set_value(string{ARGC.data(), ARGC.size()}, static_cast<int64_t>(1));
    server.set_value(string{ARGV.data(), ARGV.size()}, uri_query);
  } else if (http_server_instance_st.http_method == method::post) {
    string body_str{invoke_http.body.data(), static_cast<string::size_type>(invoke_http.body.size())};
    if (content_type == CONTENT_TYPE_APP_FORM_URLENCODED) {
      f$parse_str(body_str, superglobals.v$_POST);
    } else if (content_type == CONTENT_TYPE_MULTIPART_FORM_DATA) {
      php_error("unsupported content-type: %s", CONTENT_TYPE_MULTIPART_FORM_DATA.data());
    } else {
      http_server_instance_st.opt_raw_post_data.emplace(std::move(body_str));
    }

    server.set_value(string{CONTENT_TYPE.data(), CONTENT_TYPE.size()}, string{content_type.data(), static_cast<string::size_type>(content_type.size())});
  }

  { // set v$_REQUEST
    array<mixed> request{};
    request += superglobals.v$_GET.to_array();
    request += superglobals.v$_POST.to_array();
    request += superglobals.v$_COOKIE.to_array();
    superglobals.v$_REQUEST = std::move(request);
  }

  // ==================================
  // prepare some response headers

  // add content-type header
  auto &static_SB{RuntimeContext::get().static_SB};
  static_SB.clean() << headers::CONTENT_TYPE.data() << ": " << CONTENT_TYPE_TEXT_WIN1251.data();
  header({static_SB.c_str(), static_SB.size()}, true, status::NO_STATUS);
  // add connection kind header
  const auto connection_kind{http_server_instance_st.connection_kind == connection_kind::keep_alive ? CONNECTION_KEEP_ALIVE : CONNECTION_CLOSE};
  static_SB.clean() << headers::CONNECTION.data() << ": " << connection_kind.data();
}

task_t<void> finalize_server(const string_buffer &output) noexcept {
  auto &http_server_instance_st{HttpServerInstanceState::get()};

  string body{};
  if (http_server_instance_st.http_method != method::head) {
    body = output.str();
    const bool gzip_encoded{static_cast<bool>(http_server_instance_st.encoding & HttpServerInstanceState::ENCODING_GZIP)};
    const bool deflate_encoded{static_cast<bool>(http_server_instance_st.encoding & HttpServerInstanceState::ENCODING_DEFLATE)};
    // compress body if needed
    if (gzip_encoded || deflate_encoded) {
      auto encoded_body{kphp::zlib::encode({body.c_str(), static_cast<size_t>(body.size())}, kphp::zlib::DEFAULT_COMPRESSION_LEVEL,
                                           gzip_encoded ? kphp::zlib::ENCODING_GZIP : kphp::zlib::ENCODING_DEFLATE)};
      if (encoded_body.has_value()) [[likely]] {
        body = std::move(*encoded_body);

        auto &static_SB{RuntimeContext::get().static_SB};
        static_SB.clean() << headers::CONTENT_ENCODING.data() << ": " << (gzip_encoded ? ENCODING_GZIP.data() : ENCODING_DEFLATE.data());
        header({static_SB.c_str(), static_SB.size()}, true, status::NO_STATUS);
      }
    }
  }

  const auto status_code{http_server_instance_st.status_code == status::NO_STATUS ? status::OK : http_server_instance_st.status_code};
  tl::httpResponse http_response{.version = tl::HttpVersion{.version = tl::HttpVersion::Version::V11},
                                 .status_code = static_cast<int32_t>(status_code),
                                 .headers = {},
                                 .body = std::move(body)};
  // fill headers
  http_response.headers.data.reserve(http_server_instance_st.headers().size());
  std::transform(http_server_instance_st.headers().cbegin(), http_server_instance_st.headers().cend(), std::back_inserter(http_response.headers.data),
                 [](const auto &header_entry) noexcept {
                   const auto &[name, value]{header_entry};
                   return tl::httpHeaderEntry{.is_sensitive = {}, .name = {name.data(), name.size()}, .value = {value.data(), value.size()}};
                 });

  tl::TLBuffer tlb{};
  tl::HttpResponse{.http_response = std::move(http_response)}.store(tlb);

  auto &instance_st{InstanceState::get()};
  if ((co_await write_all_to_stream(instance_st.standard_stream(), tlb.data(), tlb.size())) != tlb.size()) [[unlikely]] {
    instance_st.poll_status = k2::PollStatus::PollFinishedError;
    php_warning("can't write component result to stream %" PRIu64, instance_st.standard_stream());
  }
}

} // namespace http

} // namespace kphp
