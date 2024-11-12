// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/server/http/init-functions.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <optional>
#include <string_view>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/core/globals/php-script-globals.h"
#include "runtime-light/server/http/http-server-state.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"

namespace {

enum class HttpConnectionKind : uint8_t { KeepAlive, Close };

enum class HttpMethod : uint8_t { GET, POST, HEAD, OTHER };

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

constexpr std::string_view HEADER_HOST = "host";
constexpr std::string_view HEADER_COOKIE = "cookie";
constexpr std::string_view HEADER_AUTHORIZATION = "authorization";
constexpr std::string_view HEADER_ACCEPT_ENCODING = "accept-encoding";

constexpr std::string_view GET_METHOD = "GET";
constexpr std::string_view POST_METHOD = "POST";
constexpr std::string_view HEAD_METHOD = "HEAD";

constexpr std::string_view ENCODING_GZIP = "gzip";
constexpr std::string_view ENCODING_DEFLATE = "deflate";

[[maybe_unused]] constexpr std::string_view CONTENT_TYPE = "CONTENT_TYPE";

string get_server_protocol(tl::HttpVersion http_version, const std::optional<string> &opt_scheme) noexcept {
  std::string_view protocol_name{};
  const auto protocol_version{http_version.string_view()};
  if (opt_scheme.has_value()) {
    const std::string_view scheme_view{(*opt_scheme).c_str(), (*opt_scheme).size()};
    if (scheme_view == HTTP_SCHEME) {
      protocol_name = HTTP;
    } else if (scheme_view == HTTPS_SCHEME) {
      protocol_name = HTTPS;
    } else {
      php_warning("unexpected http scheme: %s", scheme_view.data());
      protocol_name = HTTP;
    }
  } else {
    protocol_name = HTTP;
  }
  string protocol{};
  protocol.reserve_at_least(protocol_name.size() + protocol_version.size() + 1); // +1 for '/'
  protocol.append(protocol_name.data(), protocol_name.size());
  protocol.append(1, '/');
  protocol.append(protocol_version.data(), protocol_version.size());
  return protocol;
}

void process_cookie_header(const string &header, PhpScriptBuiltInSuperGlobals &superglobals) noexcept {
  // *** be careful here ***
  auto *cookie_start{const_cast<char *>(header.c_str())};
  auto *cookie_list_end{const_cast<char *>(header.c_str() + header.size())};
  do {
    auto *cookie_end{std::find(cookie_start, cookie_list_end, ';')};
    char *cookie_domain_end{std::find(cookie_start, cookie_end, '=')};
    if (cookie_domain_end != cookie_end) [[likely]] {
      char *cookie_value_start{std::next(cookie_domain_end)};
      const auto cookie_domain_size{static_cast<string::size_type>(std::distance(cookie_start, cookie_domain_end))};
      const auto cookie_value_size{static_cast<string::size_type>(std::distance(cookie_value_start, cookie_end))};
      string cookie_domain{cookie_start, cookie_domain_size};
      string cookie_value{cookie_value_start, cookie_value_size};
      superglobals.v$_COOKIE.set_value(cookie_domain, std::move(cookie_value)); // TODO use proper setter
    }
    // skip ';' if possible
    cookie_start = cookie_end == cookie_list_end ? cookie_list_end : std::next(cookie_end);
    // skip whitespaces
    while (cookie_start != cookie_list_end && *cookie_start == ' ') {
      cookie_start = std::next(cookie_start);
    }
  } while (cookie_start != cookie_list_end);
}

void process_headers(tl::dictionary<tl::httpHeaderValue> &&headers, PhpScriptBuiltInSuperGlobals &superglobals) noexcept {
  auto &server{superglobals.v$_SERVER};
  auto &http_server_ctx{HttpServerInstanceState::get()};
  using namespace PhpServerSuperGlobalIndices;

  // platform provides headers that are already in lowercase
  for (auto &[header_name, header] : headers) {
    const std::string_view header_name_view{header_name.c_str(), header_name.size()};
    const std::string_view header_view{header.value.c_str(), header.value.size()};

    if (header_name_view == HEADER_ACCEPT_ENCODING) {
      if (header_view.find(ENCODING_GZIP) != std::string_view::npos) {
        http_server_ctx.encoding |= HttpServerInstanceState::ENCODING_GZIP;
      }
      if (header_view.find(ENCODING_DEFLATE) != std::string_view::npos) {
        http_server_ctx.encoding |= HttpServerInstanceState::ENCODING_DEFLATE;
      }
    } else if (header_name_view == HEADER_COOKIE) {
      process_cookie_header(header.value, superglobals);
    } else if (header_name_view == HEADER_HOST) {
      server.set_value(string{SERVER_NAME.data(), SERVER_NAME.size()}, header.value);
    } else if (header_name_view == HEADER_AUTHORIZATION) {
      php_warning("authorization not implemented"); // TODO parse authorization
    }
    // add header entries
    string key{};
    key.reserve_at_least(HTTP_HEADER_PREFIX.size() + header_name.size());
    key.append(HTTP_HEADER_PREFIX.data());
    key.append(header_name_view.data(), header_name_view.size());
    // to uppercase inplace
    for (int64_t i = HTTP_HEADER_PREFIX.size(); i < key.size(); ++i) {
      key[i] = key[i] != '-' ? std::toupper(key[i]) : '_';
    }
    server.set_value(key, std::move(header.value));
  }
}

} // namespace

void init_http_server(tl::K2InvokeHttp &&invoke_http) noexcept {
  auto &superglobals{InstanceState::get().php_script_mutable_globals_singleton.get_superglobals()};
  auto &server{superglobals.v$_SERVER};

  HttpMethod http_method{HttpMethod::OTHER};
  const std::string_view http_method_view{invoke_http.method.c_str(), invoke_http.method.size()};
  if (http_method_view == GET_METHOD) {
    http_method = HttpMethod::GET;
  } else if (http_method_view == POST_METHOD) {
    http_method = HttpMethod::POST;
  } else if (http_method_view == HEAD_METHOD) {
    http_method = HttpMethod::HEAD;
  }

  using namespace PhpServerSuperGlobalIndices;
  if (http_method == HttpMethod::GET) {
    server.set_value(string{ARGC.data(), ARGC.size()}, static_cast<int64_t>(1));
    server.set_value(string{ARGV.data(), ARGV.size()}, invoke_http.uri.opt_query.value_or(string{}));
  }
  server.set_value(string{PHP_SELF.data(), PHP_SELF.size()}, invoke_http.uri.path);
  server.set_value(string{SCRIPT_NAME.data(), SCRIPT_NAME.size()}, invoke_http.uri.path);
  server.set_value(string{SCRIPT_URL.data(), SCRIPT_URL.size()}, invoke_http.uri.path);

  server.set_value(string{SERVER_ADDR.data(), SERVER_ADDR.size()}, invoke_http.connection.server_addr);
  server.set_value(string{SERVER_PORT.data(), SERVER_PORT.size()}, static_cast<int64_t>(invoke_http.connection.server_port));
  server.set_value(string{SERVER_PROTOCOL.data(), SERVER_PROTOCOL.size()}, get_server_protocol(invoke_http.version, invoke_http.uri.opt_scheme));
  server.set_value(string{REMOTE_ADDR.data(), REMOTE_ADDR.size()}, invoke_http.connection.remote_addr);
  server.set_value(string{REMOTE_PORT.data(), REMOTE_PORT.size()}, static_cast<int64_t>(invoke_http.connection.remote_port));

  server.set_value(string{REQUEST_METHOD.data(), REQUEST_METHOD.size()}, invoke_http.method);
  server.set_value(string{GATEWAY_INTERFACE.data(), GATEWAY_INTERFACE.size()}, string{GATEWAY_INTERFACE_VALUE.data(), GATEWAY_INTERFACE_VALUE.size()});

  if (invoke_http.uri.opt_scheme.has_value()) {
    const std::string_view scheme_view{(*invoke_http.uri.opt_scheme).c_str(), (*invoke_http.uri.opt_scheme).size()};
    if (scheme_view == HTTPS_SCHEME) {
      server.set_value(string{HTTPS.data(), HTTPS.size()}, true);
    }
  }

  if (invoke_http.uri.opt_query.has_value()) {
    // TODO v_GET
    php_warning("v_GET not implemented");
    server.set_value(string{QUERY_STRING.data(), QUERY_STRING.size()}, *invoke_http.uri.opt_query);

    string uri_string{};
    uri_string.reserve_at_least((invoke_http.uri.path.size()) + (*invoke_http.uri.opt_query).size() + 1); // +1 for '?'
    uri_string.append(invoke_http.uri.path);
    uri_string.append(1, '?');
    uri_string.append(*invoke_http.uri.opt_query);
    server.set_value(string{REQUEST_URI.data(), REQUEST_URI.size()}, uri_string);
  } else {
    server.set_value(string{REQUEST_URI.data(), REQUEST_URI.size()}, invoke_http.uri.path);
  }

  process_headers(std::move(invoke_http.headers), superglobals);
  // TODO connection_kind, CONTENT_TYPE, is_head_query, _REQUEST, v_POST
  php_warning("conncetion kind, content type, post and head methods, _REQUEST superglobal are not supported yet");

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
}
