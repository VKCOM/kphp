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
#include <string>
#include <utility>

#include "runtime-core/runtime-core.h"
#include "runtime-light/component/component.h"
#include "runtime-light/core/globals/php-script-globals.h"
#include "runtime-light/server/http/http-server-context.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"
#include "runtime-light/utils/context.h"

namespace {

enum class HttpConnectionKind : uint8_t { KeepAlive, Close };

enum class HttpMethod : uint8_t { GET, POST, HEAD, OTHER };

constexpr auto *HTTPS = "HTTPS";
constexpr auto *HTTPS_SCHEME = "https";
constexpr auto *HTTP_HEADER_PREFIX = "HTTP_";
constexpr auto *HTTP_X_REAL_HOST = "HTTP_X_REAL_HOST";
constexpr auto *HTTP_X_REAL_SCHEME = "HTTP_X_REAL_SCHEME";
constexpr auto *HTTP_X_REAL_REQUEST = "HTTP_X_REAL_REQUEST";

constexpr auto *SCHEME_SUFFIX = "://";
constexpr auto *GATEWAY_INTERFACE_VALUE = "CGI/1.1";

constexpr auto *HEADER_HOST = "host";
constexpr auto *HEADER_COOKIE = "cookie";
constexpr auto *HEADER_AUTHORIZATION = "authorization";
constexpr auto *HEADER_ACCEPT_ENCODING = "accept-encoding";

constexpr auto *GET_METHOD = "GET";
constexpr auto *POST_METHOD = "POST";
constexpr auto *HEAD_METHOD = "HEAD";

constexpr auto *ENCODING_GZIP = "gzip";
constexpr auto *ENCODING_DEFLATE = "deflate";

[[maybe_unused]] constexpr auto *CONTENT_TYPE = "CONTENT_TYPE";

string get_server_protocol([[maybe_unused]] tl::HttpVersion http_version, [[maybe_unused]] const std::optional<string> &opt_scheme) noexcept {
  return string{"HTTP/1.1"}; // TODO
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
  auto &http_server_ctx{HttpServerComponentContext::get()};
  using namespace PhpServerSuperGlobalIndices;

  // platform provides headers that are already in lowercase
  for (auto &[header_name, header] : headers) {
    if (std::strcmp(header_name.c_str(), HEADER_ACCEPT_ENCODING) == 0) {
      if (std::strstr(header.value.c_str(), ENCODING_GZIP) != nullptr) {
        http_server_ctx.encoding |= HttpServerComponentContext::ENCODING_GZIP;
      }
      if (std::strstr(header.value.c_str(), ENCODING_DEFLATE) != nullptr) {
        http_server_ctx.encoding |= HttpServerComponentContext::ENCODING_DEFLATE;
      }
    } else if (std::strcmp(header_name.c_str(), HEADER_COOKIE) == 0) {
      process_cookie_header(header.value, superglobals);
    } else if (std::strcmp(header_name.c_str(), HEADER_HOST) == 0) {
      server.set_value(string{SERVER_NAME, std::char_traits<char>::length(SERVER_NAME)}, header.value);
    } else if (std::strcmp(header_name.c_str(), HEADER_AUTHORIZATION) == 0) {
      // TODO parse authorization
    }
    // add header entries
    string key{};
    key.reserve_at_least(std::char_traits<char>::length(HTTP_HEADER_PREFIX) + header_name.size());
    key.append(HTTP_HEADER_PREFIX);
    key.append(header_name.c_str());
    // to uppercase inplace
    for (int64_t i = std::char_traits<char>::length(HTTP_HEADER_PREFIX); i < key.size(); ++i) {
      key[i] = key[i] != '-' ? std::toupper(key[i]) : '_';
    }
    server.set_value(key, std::move(header.value));
  }
}

} // namespace

void init_http_server(tl::K2InvokeHttp &&invoke_http) noexcept {
  auto &superglobals{get_component_context()->php_script_mutable_globals_singleton.get_superglobals()};
  auto &server{superglobals.v$_SERVER};

  HttpMethod http_method{HttpMethod::OTHER};
  if (std::strcmp(invoke_http.method.c_str(), GET_METHOD) == 0) {
    http_method = HttpMethod::GET;
  } else if (std::strcmp(invoke_http.method.c_str(), POST_METHOD) == 0) {
    http_method = HttpMethod::POST;
  } else if (std::strcmp(invoke_http.method.c_str(), HEAD_METHOD) == 0) {
    http_method = HttpMethod::HEAD;
  }

  using namespace PhpServerSuperGlobalIndices;
  if (http_method == HttpMethod::GET) {
    server.set_value(string{ARGC, std::char_traits<char>::length(ARGC)}, static_cast<int64_t>(1));
    server.set_value(string{ARGV, std::char_traits<char>::length(ARGV)}, invoke_http.uri.opt_query.value_or(string{}));
  }
  server.set_value(string{PHP_SELF, std::char_traits<char>::length(PHP_SELF)}, invoke_http.uri.path);
  server.set_value(string{SCRIPT_NAME, std::char_traits<char>::length(SCRIPT_NAME)}, invoke_http.uri.path);
  server.set_value(string{SCRIPT_URL, std::char_traits<char>::length(SCRIPT_URL)}, invoke_http.uri.path);

  server.set_value(string{SERVER_ADDR, std::char_traits<char>::length(SERVER_ADDR)}, invoke_http.connection.server_addr);
  server.set_value(string{SERVER_PORT, std::char_traits<char>::length(SERVER_PORT)}, static_cast<int64_t>(invoke_http.connection.server_port));
  server.set_value(string{SERVER_PROTOCOL, std::char_traits<char>::length(SERVER_PROTOCOL)},
                   get_server_protocol(invoke_http.version, invoke_http.uri.opt_scheme));
  server.set_value(string{REMOTE_ADDR, std::char_traits<char>::length(REMOTE_ADDR)}, invoke_http.connection.remote_addr);
  server.set_value(string{REMOTE_PORT, std::char_traits<char>::length(REMOTE_PORT)}, static_cast<int64_t>(invoke_http.connection.remote_port));

  server.set_value(string{REQUEST_METHOD, std::char_traits<char>::length(REQUEST_METHOD)}, invoke_http.method);
  server.set_value(string{GATEWAY_INTERFACE, std::char_traits<char>::length(GATEWAY_INTERFACE)},
                   string{GATEWAY_INTERFACE_VALUE, std::char_traits<char>::length(GATEWAY_INTERFACE_VALUE)});

  if (invoke_http.uri.opt_scheme.has_value() && std::strcmp((*invoke_http.uri.opt_scheme).c_str(), HTTPS_SCHEME) == 0) {
    server.set_value(string{HTTPS, std::char_traits<char>::length(HTTPS)}, true);
  }

  if (invoke_http.uri.opt_query.has_value()) {
    // TODO v_GET
    server.set_value(string{QUERY_STRING, std::char_traits<char>::length(QUERY_STRING)}, *invoke_http.uri.opt_query);

    string uri_string{};
    uri_string.reserve_at_least((invoke_http.uri.path.size()) + (*invoke_http.uri.opt_query).size() + 1); // +1 for '?'
    uri_string.append(invoke_http.uri.path);
    uri_string.append(1, '?');
    uri_string.append(*invoke_http.uri.opt_query);
    server.set_value(string{REQUEST_URI, std::char_traits<char>::length(REQUEST_URI)}, uri_string);
  } else {
    server.set_value(string{REQUEST_URI, std::char_traits<char>::length(REQUEST_URI)}, invoke_http.uri.path);
  }

  process_headers(std::move(invoke_http.headers), superglobals);
  // TODO connection_kind, CONTENT_TYPE, is_head_query, _REQUEST, v_POST

  string real_scheme_header{HTTP_X_REAL_SCHEME, std::char_traits<char>::length(HTTP_X_REAL_SCHEME)};
  string real_host_header{HTTP_X_REAL_HOST, std::char_traits<char>::length(HTTP_X_REAL_HOST)};
  string real_request_header{HTTP_X_REAL_REQUEST, std::char_traits<char>::length(HTTP_X_REAL_REQUEST)};
  if (server.isset(real_scheme_header) || server.isset(real_host_header) || server.isset(real_request_header)) {
    string real_scheme{server.get_value(real_request_header).to_string()};
    string real_host{server.get_value(real_host_header).to_string()};
    string real_request{server.get_value(real_request_header).to_string()};

    string script_uri{};
    script_uri.reserve_at_least(real_scheme.size() + std::char_traits<char>::length(SCHEME_SUFFIX) + real_host.size() + real_request.size());
    script_uri.append(real_scheme);
    script_uri.append(SCHEME_SUFFIX, std::char_traits<char>::length(SCHEME_SUFFIX));
    script_uri.append(real_host);
    script_uri.append(real_request);
    server.set_value(string{SCRIPT_URI, std::char_traits<char>::length(SCRIPT_URI)}, script_uri);
  }
}
