// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/server/http-functions.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <locale>
#include <memory>
#include <optional>
#include <ranges>
#include <string_view>
#include <utility>

#include "zlib/zlib.h"

#include "common/algorithms/string-algorithms.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/coroutine-state.h"
#include "runtime-light/coroutine/shared-task.h"
#include "runtime-light/server/http/http-server-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/fork/fork-functions.h"
#include "runtime-light/stdlib/output/output-state.h"
#include "runtime-light/stdlib/system/system-functions.h"
#include "runtime-light/stdlib/time/time-functions.h"
#include "runtime-light/stdlib/zlib/zlib-functions.h"
#include "runtime-light/streams/connection.h"
#include "runtime-light/streams/stream.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-types.h"

namespace {

constexpr std::string_view HTTP_DATE = R"(D, d M Y H:i:s \G\M\T)";
constexpr std::string_view HTTP_STATUS_PREFIX = "http/";

bool http_status_header(std::string_view header) noexcept {
  const auto lowercase_prefix{header | std::views::take(HTTP_STATUS_PREFIX.size()) |
                              std::views::transform([](auto c) noexcept { return std::tolower(c, std::locale::classic()); })};
  return std::ranges::equal(lowercase_prefix, HTTP_STATUS_PREFIX);
}

bool http_location_header(std::string_view header) noexcept {
  {
    auto [name_view, value_view]{vk::split_string_view(header, ':')};
    if (name_view.size() + value_view.size() + 1 != header.size()) [[unlikely]] {
      return false;
    }
  }

  const auto lowercase_prefix{header | std::views::take(kphp::http::headers::LOCATION.size()) |
                              std::views::transform([](auto c) noexcept { return std::tolower(c, std::locale::classic()); })};
  return std::ranges::equal(lowercase_prefix, kphp::http::headers::LOCATION);
}

std::optional<uint64_t> valid_http_status_header(std::string_view header) noexcept {
  const auto header_size{header.size()};
  uint64_t http_response_code{};

  if (header_size < HTTP_STATUS_PREFIX.size()) [[unlikely]] {
    return {};
  }
  // skip digits
  size_t pos{HTTP_STATUS_PREFIX.size()};
  for (; pos < header_size && std::isdigit(header[pos], std::locale::classic()); ++pos) {
  }
  // expect '.'
  if (pos == header_size || header[pos++] != '.') [[unlikely]] {
    return {};
  }
  // skip digits
  for (; pos < header_size && std::isdigit(header[pos], std::locale::classic()); ++pos) {
  }
  // expect ' '
  if (pos == header_size || header[pos++] != ' ') [[unlikely]] {
    return {};
  }
  // expect 3 digits http code
  if (pos == header_size || !std::isdigit(header[pos], std::locale::classic())) [[unlikely]] {
    return {};
  }
  http_response_code = header[pos++] - '0';
  if (pos == header_size || !std::isdigit(header[pos], std::locale::classic())) [[unlikely]] {
    return {};
  }
  http_response_code *= 10;
  http_response_code += header[pos++] - '0';
  if (pos == header_size || !std::isdigit(header[pos], std::locale::classic())) [[unlikely]] {
    return {};
  }
  http_response_code *= 10;
  http_response_code += header[pos++] - '0';
  // expect ' '
  if (pos == header_size || header[pos++] != ' ') [[unlikely]] {
    return {};
  }
  // expect all remaining characters to be printable
  for (; pos < header_size; ++pos) {
    if (std::isprint(header[pos], std::locale::classic()) == 0) [[unlikely]] {
      return {};
    }
  }
  return http_response_code;
}

// flush() consumes only the system buffer; finalization also drains open user buffers.
string prepare_http_response_body(HttpServerInstanceState& state, bool finish) noexcept {
  auto& buffers{OutputInstanceState::get().output_buffers};
  auto& system_buffer{buffers.system_buffer().get()};
  string body{};
  if (state.http_method != kphp::http::method::head) {
    const auto user_buffers{buffers.user_buffers().first(finish ? buffers.user_level() : 0)};
    const auto body_size{std::ranges::fold_left(user_buffers | std::views::transform([](const auto& buffer) noexcept { return buffer.size(); }),
                                                system_buffer.size(), std::plus<string::size_type>{})};
    body.reserve_at_least(body_size);

    body.append(system_buffer.buffer(), system_buffer.size());
    const auto appender{[&body](const auto& buffer) noexcept { body.append(buffer.buffer(), buffer.size()); }};
    std::ranges::for_each(user_buffers | std::views::filter([](const auto& buffer) noexcept { return buffer.size() > 0; }), appender);
  }
  system_buffer.clean();

  if (state.response_encoding != 0 && state.http_method != kphp::http::method::head) {
    auto compressed{state.body_compressor.compress({body.c_str(), static_cast<size_t>(body.size())}, finish)};
    if (!compressed) [[unlikely]] {
      kphp::log::error("can't compress HTTP response");
    }
    body = std::move(*compressed);
  }
  if (finish) {
    state.body_compressor.close();
  }
  return body;
}

// Freeze content encoding together with the headers, independently of later ob_* calls.
tl::HttpResponseHeader build_response_header(HttpServerInstanceState& state) noexcept {
  const bool auto_encoding_enabled{state.auto_encoding_enabled};
  const bool gzip_encoded{static_cast<bool>(state.encoding & HttpServerInstanceState::ENCODING_GZIP)};
  const bool deflate_encoded{static_cast<bool>(state.encoding & HttpServerInstanceState::ENCODING_DEFLATE)};

  if (auto_encoding_enabled && (gzip_encoded || deflate_encoded)) {
    state.response_encoding = gzip_encoded ? HttpServerInstanceState::ENCODING_GZIP : HttpServerInstanceState::ENCODING_DEFLATE;
    const auto encoding{gzip_encoded ? kphp::zlib::ENCODING_GZIP : kphp::zlib::ENCODING_DEFLATE};
    if (!state.body_compressor.init_if_needed(kphp::zlib::DEFAULT_COMPRESSION_LEVEL, encoding, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY)) [[unlikely]] {
      kphp::log::error("can't initialize HTTP response compression");
    }
    kphp::http::header(gzip_encoded ? "Content-Encoding: gzip" : "Content-Encoding: deflate", /* replace = */ true, kphp::http::status::NO_STATUS);
  }

  tl::HttpResponseHeader header{};
  const auto status_code{state.status_code == kphp::http::status::NO_STATUS ? kphp::http::status::OK : state.status_code};
  header.inner.version = tl::HttpVersion{.version = tl::HttpVersion::Version::V11};
  header.inner.status_code = {.value = static_cast<int32_t>(status_code)};
  header.inner.headers.value.reserve(state.headers().size());
  std::transform(state.headers().cbegin(), state.headers().cend(), std::back_inserter(header.inner.headers.value), [](const auto& header_entry) noexcept {
    const auto& [name, value]{header_entry};
    return tl::httpHeaderEntry{.is_sensitive = {}, .name = {.value = {name.data(), name.size()}}, .value = {.value = {value.data(), value.size()}}};
  });
  return header;
}

// event::set() resumes waiters inline, which can change both the fork ID and the async stack.
void notify_headers_ready(HttpServerInstanceState& state) noexcept {
  if (state.headers_ready.has_value()) {
    const kphp::forks::scoped_id_managed restore_fork_id{};
    auto& stack{kphp::coro::instance_state::get().coroutine_stack_root};
    auto* previous_frame{stack.top_async_stack_frame};
    state.headers_ready->set();
    stack.top_async_stack_frame = previous_frame;
  }
}

kphp::coro::task<> wait_headers_ready(HttpServerInstanceState& state) noexcept {
  if (!state.headers_ready.has_value()) {
    co_return;
  }
  co_await *state.headers_ready;
}

kphp::coro::shared_task<> run_headers_callback(HttpServerInstanceState& state, kphp::coro::task<> callback) noexcept {
  auto& fork{ForkInstanceState::get().current_info().get()};
  const void* previous_context{std::exchange(fork.callback_context, std::addressof(state))};
  co_await kphp::forks::id_managed(std::move(callback));
  fork.callback_context = previous_context;
  notify_headers_ready(state);
}

// Serialize writers across suspension points so concurrent forks cannot interleave TL packets.
// Callbacks run before joining this queue: a callback may itself call flush().
kphp::coro::shared_task<> write_response(HttpServerInstanceState& state, std::optional<kphp::coro::shared_task<>> previous, bool finish) noexcept {
  if (previous.has_value()) {
    co_await kphp::forks::id_managed(previous->when_ready());
    previous.reset();
  }
  if (state.response_finished || state.connection->is_aborted()) {
    OutputInstanceState::get().output_buffers.system_buffer().get().clean();
    co_return;
  }

  tl::HttpResponseChunk chunk{};
  if (!state.headers_sent) {
    chunk.opt_header = build_response_header(state);
    state.headers_sent = true;
    notify_headers_ready(state);
  }
  string body{prepare_http_response_body(state, finish)};
  chunk.body.inner.body = {reinterpret_cast<const std::byte*>(body.c_str()), body.size()};
  chunk.last = finish;
  tl::storer tls{chunk.footprint()};
  chunk.store(tls);

  auto& stream{state.connection->get_stream()};
  if (auto expected{co_await kphp::forks::id_managed(stream.write_all(tls.view()))}; !expected) [[unlikely]] {
    const bool aborted{expected.error() == k2::errno_eshutdown || state.connection->is_aborted()};
    state.response_finished = true;
    stream.shutdown_write();
    if (aborted) {
      if (state.connection->get_ignore_abort_level() == 0) {
        co_await kphp::forks::id_managed(kphp::system::exit(1));
      }
      co_return;
    }
    kphp::log::error("can't write HTTP response: error code -> {}", expected.error());
  }
  if (finish) {
    state.response_finished = true;
    stream.shutdown_write();
  }
}

} // namespace

namespace kphp::http {

kphp::coro::task<> invoke_headers_callback(HttpServerInstanceState& state, bool finish) noexcept {
  if (!std::exchange(state.headers_callback_started, true) && state.headers_registered_callback.has_value()) {
    auto callback{std::exchange(state.headers_registered_callback, std::nullopt)};
    state.headers_ready.emplace();
    state.headers_callback_task.emplace(run_headers_callback(state, std::move(*callback)));
    co_await kphp::forks::id_managed(*state.headers_callback_task);
    co_return;
  }
  if (!state.headers_callback_task.has_value() || ForkInstanceState::get().current_info().get().callback_context == std::addressof(state)) {
    // The callback and forks created inside it may flush or exit without waiting for themselves.
    co_return;
  }
  if (finish) {
    co_await kphp::forks::id_managed(*state.headers_callback_task);
  } else if (!state.headers_sent) {
    // An independent fork must not commit headers while the callback is still preparing them.
    // Before awaiting such a fork, the callback must explicitly flush to release this gate.
    co_await kphp::forks::id_managed(wait_headers_ready(state));
  }
}

kphp::coro::task<> send_response(HttpServerInstanceState& state, bool finish) noexcept {
  auto writer{write_response(state, std::exchange(state.pending_response, std::nullopt), finish)};
  state.pending_response.emplace(writer);
  co_await kphp::forks::id_managed(std::move(writer));
}

void header(std::string_view header_view, bool replace, int64_t response_code) noexcept {
  if (response_code < 0) [[unlikely]] {
    return kphp::log::warning("http response code can't be negative: {}", response_code);
  }

  auto& http_server_instance_st{HttpServerInstanceState::get()};
  if (http_server_instance_st.headers_sent) {
    return;
  }

  // HTTP status special case
  if (http_status_header(header_view)) {
    if (const auto opt_status_code{valid_http_status_header(header_view)}; opt_status_code.has_value()) [[likely]] {
      http_server_instance_st.status_code = *opt_status_code;
      return;
    } else {
      kphp::log::error("invalid HTTP status header: {}", header_view);
    }
  }

  auto [name_view, value_view]{vk::split_string_view(header_view, ':')};
  if (name_view.size() + value_view.size() + 1 != header_view.size()) [[unlikely]] {
    return kphp::log::warning("invalid header: {}", header_view);
  }

  // validate header name
  name_view = vk::strip_ascii_whitespace(name_view);
  if (!std::ranges::all_of(name_view, [](char c) noexcept { return std::isalnum(c, std::locale::classic()) || c == '-' || c == '_'; })) [[unlikely]] {
    return kphp::log::warning("invalid header name: {}", name_view);
  }
  // validate header value
  value_view = vk::strip_ascii_whitespace(value_view);
  if (!std::ranges::all_of(value_view, [](char c) noexcept { return std::isprint(c, std::locale::classic()); })) [[unlikely]] {
    return kphp::log::warning("invalid header value: {}", value_view);
  }

  http_server_instance_st.add_header(name_view, value_view, replace);

  // Location: special case
  const bool can_return_redirect{
      response_code == status::NO_STATUS && http_server_instance_st.status_code != status::CREATED &&
      (http_server_instance_st.status_code < status::MULTIPLE_CHOICES || http_server_instance_st.status_code >= status::BAD_REQUEST)};
  if (can_return_redirect && http_location_header(header_view)) {
    http_server_instance_st.status_code = status::FOUND;
  }

  if (!header_view.empty() && response_code != status::NO_STATUS) {
    http_server_instance_st.status_code = response_code;
  }
}

} // namespace kphp::http

void f$setrawcookie(const string& name, const string& value, int64_t expire_or_options, const string& path, const string& domain, bool secure,
                    bool http_only) noexcept {
  auto& static_SB_spare{RuntimeContext::get().static_SB_spare};

  static_SB_spare.clean() << kphp::http::headers::SET_COOKIE.data() << ": " << name << '=';
  if (value.empty()) {
    static_SB_spare << "DELETED; expires=Thu, 01 Jan 1970 00:00:01 GMT";
  } else {
    static_SB_spare << value;
    if (expire_or_options != 0) {
      static_SB_spare << "; expires=" << f$gmdate({HTTP_DATE.data(), HTTP_DATE.size()}, expire_or_options);
    }
  }
  if (!path.empty()) {
    static_SB_spare << "; path=" << path;
  }
  if (!domain.empty()) {
    static_SB_spare << "; domain=" << domain;
  }
  if (secure) {
    static_SB_spare << "; secure";
  }
  if (http_only) {
    static_SB_spare << "; HttpOnly";
  }
  kphp::http::header({static_SB_spare.c_str(), static_SB_spare.size()}, false, kphp::http::status::NO_STATUS);
}
