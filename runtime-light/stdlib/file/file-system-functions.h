// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <sys/stat.h>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/array/array-functions.h"
#include "runtime-common/stdlib/string/string-functions.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/file/resource.h"
#include "runtime-light/stdlib/fork/fork-functions.h"

namespace file_system_impl_ {

inline constexpr char SEPARATOR = '/';

} // namespace file_system_impl_

inline constexpr int64_t STREAM_CLIENT_CONNECT = 1;
inline constexpr int64_t DEFAULT_SOCKET_TIMEOUT = 60;

// *** ATTENTION ***
// For some reason KPHP's implementation of basename works incorrectly for at least following cases:
// 1. basename("");
// 2. basename("/");
//
// This implementation works the same way as PHP. That means we can face with some problems
// during transition to K2
inline string f$basename(const string& path, const string& suffix = {}) noexcept {
  std::string_view path_view{path.c_str(), path.size()};
  const std::string_view suffix_view{suffix.c_str(), suffix.size()};
  // skip trailing separators
  while (!path_view.empty() && path_view.back() == file_system_impl_::SEPARATOR) {
    path_view.remove_suffix(1);
  }
  const auto last_separator_pos{path_view.find_last_of(file_system_impl_::SEPARATOR)};
  std::string_view filename_view{last_separator_pos == std::string_view::npos ? path_view : path_view.substr(last_separator_pos + 1)};

  if (!suffix_view.empty() && filename_view.size() >= suffix_view.size() && filename_view.ends_with(suffix_view)) {
    filename_view.remove_suffix(suffix_view.size());
  }

  return {filename_view.data(), static_cast<string::size_type>(filename_view.size())};
}

inline Optional<int64_t> f$filesize(const string& filename) noexcept {
  struct stat stat {};
  if (auto errc{k2::stat({filename.c_str(), filename.size()}, std::addressof(stat))}; errc != k2::errno_ok) [[unlikely]] {
    return false;
  }
  return static_cast<int64_t>(stat.st_size);
}

inline Optional<string> f$realpath(const string& path) noexcept {
  auto expected_path{k2::canonicalize({path.c_str(), path.size()})};
  if (!expected_path) [[unlikely]] {
    return false;
  }

  // *** important ***
  // canonicalized_path is **not** null-terminated
  auto [canonicalized_path, size]{*std::move(expected_path)};
  return string{canonicalized_path.get(), static_cast<string::size_type>(size)};
}

inline resource f$fopen(const string& filename, const string& mode, [[maybe_unused]] bool use_include_path = false,
                        [[maybe_unused]] const resource& context = {}) noexcept {
  std::string_view filename_view{filename.c_str(), filename.size()};
  if (filename_view == kphp::fs::STDIN_NAME) {
    auto expected{kphp::fs::stdinput::open()};
    return expected ? make_instance<kphp::fs::stdinput>(*std::move(expected)) : class_instance<kphp::fs::stdinput>{};
  } else if (filename_view == kphp::fs::STDOUT_NAME) {
    auto expected{kphp::fs::stdoutput::open()};
    return expected ? make_instance<kphp::fs::stdoutput>(*std::move(expected)) : class_instance<kphp::fs::stdoutput>{};
  } else if (filename_view == kphp::fs::STDERR_NAME) {
    auto expected{kphp::fs::stderror::open()};
    return expected ? make_instance<kphp::fs::stderror>(*std::move(expected)) : class_instance<kphp::fs::stderror>{};
  } else if (filename_view == kphp::fs::INPUT_NAME) {
    auto expected{kphp::fs::input::open()};
    return expected ? make_instance<kphp::fs::input>(*std::move(expected)) : class_instance<kphp::fs::input>{};
  } else if (filename_view.starts_with(kphp::fs::UDP_SCHEME_PREFIX) || filename_view.starts_with(kphp::fs::TCP_SCHEME_PREFIX)) {
    auto expected{kphp::fs::socket::open(filename_view)};
    return expected ? make_instance<kphp::fs::socket>(*std::move(expected)) : class_instance<kphp::fs::socket>{};
  } else if (!filename_view.contains(kphp::fs::SCHEME_DELIMITER)) { // not a '*://*' pattern, so it must be a file
    auto expected{kphp::fs::file::open(filename_view, {mode.c_str(), mode.size()})};
    return expected ? make_instance<kphp::fs::file>(*std::move(expected)) : class_instance<kphp::fs::file>{};
  }

  kphp::log::warning("unexpected resource in fopen -> {}", filename.c_str());
  return {};
}

inline kphp::coro::task<bool> f$fclose(resource stream) noexcept {
  if (auto sync_resource{from_mixed<class_instance<kphp::fs::sync_resource>>(stream, {})}; !sync_resource.is_null()) {
    co_return sync_resource.get()->close();
  } else if (auto async_resource{from_mixed<class_instance<kphp::fs::async_resource>>(stream, {})}; !async_resource.is_null()) {
    co_return co_await kphp::forks::id_managed(async_resource.get()->close());
  }

  kphp::log::warning("unexpected resource in fclose -> {}", stream.to_string().c_str());
  co_return false;
}

inline kphp::coro::task<Optional<int64_t>> f$fwrite(resource stream, string data) noexcept {
  std::span<const char> data_span{data.c_str(), data.size()};
  if (auto sync_resource{from_mixed<class_instance<kphp::fs::sync_resource>>(stream, {})}; !sync_resource.is_null()) {
    auto expected{sync_resource.get()->write(std::as_bytes(data_span))};
    co_return expected ? Optional<int64_t>{static_cast<int64_t>(*std::move(expected))} : Optional<int64_t>{false};
  } else if (auto async_resource{from_mixed<class_instance<kphp::fs::async_resource>>(stream, {})}; !async_resource.is_null()) {
    auto expected{co_await kphp::forks::id_managed(async_resource.get()->write(std::as_bytes(data_span)))};
    co_return expected ? Optional<int64_t>{static_cast<int64_t>(*std::move(expected))} : Optional<int64_t>{false};
  }

  kphp::log::warning("unexpected resource in fwrite -> {}", stream.to_string().c_str());
  co_return false;
}

inline kphp::coro::task<Optional<string>> f$fread(resource stream, int64_t length) noexcept {
  if (length < 0 || length > static_cast<int64_t>(std::numeric_limits<string::size_type>::max())) [[unlikely]] {
    co_return false;
  }

  string data{static_cast<string::size_type>(length), false};
  std::span<char> buf{data.buffer(), data.size()};
  std::expected<size_t, int32_t> expected{std::unexpected{k2::errno_einval}};
  if (auto sync_resource{from_mixed<class_instance<kphp::fs::sync_resource>>(stream, {})}; !sync_resource.is_null()) {
    expected = sync_resource.get()->read(std::as_writable_bytes(buf));
  } else if (auto async_resource{from_mixed<class_instance<kphp::fs::async_resource>>(stream, {})}; !async_resource.is_null()) {
    expected = co_await kphp::forks::id_managed(async_resource.get()->read(std::as_writable_bytes(buf)));
  } else {
    kphp::log::warning("unexpected resource in fread -> {}", stream.to_string().c_str());
    co_return false;
  }

  if (!expected) [[unlikely]] {
    co_return false;
  }
  co_return *expected == static_cast<size_t>(length) ? std::move(data) : f$substr(data, 0, static_cast<int64_t>(*expected));
}

inline kphp::coro::task<bool> f$fflush(resource stream) noexcept {
  if (auto sync_resource{from_mixed<class_instance<kphp::fs::sync_resource>>(stream, {})}; !sync_resource.is_null()) {
    co_return sync_resource.get()->flush();
  } else if (auto async_resource{from_mixed<class_instance<kphp::fs::async_resource>>(stream, {})}; !async_resource.is_null()) {
    co_return co_await kphp::forks::id_managed(async_resource.get()->flush());
  }

  kphp::log::warning("unexpected resource in fflush -> {}", stream.to_string().c_str());
  co_return false;
}

inline resource f$stream_socket_client(const string& address, std::optional<std::reference_wrapper<mixed>> error_code = {},
                                       std::optional<std::reference_wrapper<mixed>> /*error_message*/ = {}, double /*timeout*/ = DEFAULT_SOCKET_TIMEOUT,
                                       int64_t /*flags*/ = STREAM_CLIENT_CONNECT, const resource& /*context*/ = {}) noexcept {
  /*
   * TODO: Here should be waiting with timeout,
   *       but it can't be expressed simple ways by awaitables since we blocked inside k2
   * */
  auto expected{kphp::fs::socket::open({address.c_str(), address.size()})};
  if (!expected) [[unlikely]] {
    if (error_code.has_value()) {
      (*error_code).get() = static_cast<int64_t>(expected.error());
    }
    return {};
  }
  return make_instance<kphp::fs::socket>(*std::move(expected));
}

constexpr std::string_view READ_MODE = std::string_view{"r"};
constexpr std::string_view WRITE_MODE = std::string_view("w");
constexpr std::string_view APPEND_MODE = std::string_view("a");

inline Optional<string> f$file_get_contents(const string& stream) noexcept {
  if (auto sync_resource{
          from_mixed<class_instance<kphp::fs::sync_resource>>(f$fopen(stream, string{READ_MODE.data(), static_cast<string::size_type>(READ_MODE.size())}), {})};
      !sync_resource.is_null()) {
    auto expected{sync_resource.get()->get_contents()};
    return expected ? Optional<string>{*std::move(expected)} : Optional<string>{false};
  }
  return false;
}

namespace {
inline std::expected<size_t, int32_t> write_safe(kphp::fs::sync_resource* resource, std::span<const std::byte> src) noexcept {
  size_t full_len = src.size();
  do {
    std::expected<size_t, int32_t> cur_res = resource->write(src);
    if (!cur_res) {
      return cur_res;
    }

    src = src.subspan(*cur_res);
  } while (!src.empty());

  return std::expected<size_t, int32_t>{full_len - src.size()};
}

inline std::expected<size_t, int32_t> read_safe(kphp::fs::sync_resource* resource, std::span<std::byte> dst) noexcept {
  size_t full_len = dst.size();
  do {
    std::expected<size_t, int32_t> cur_res = resource->read(dst);
    if (!cur_res) {
      return cur_res;
    }
    if (*cur_res == 0) {
      break;
    }

    dst.subspan(*cur_res);
  } while (!dst.empty());

  return full_len - dst.size();
}
} // namespace

constexpr int64_t FILE_APPEND_FLAG = 1;

inline Optional<int64_t> f$file_put_contents(const string& stream, const mixed& content_var, int64_t flags = 0) noexcept {
  string content;
  if (content_var.is_array()) {
    content = f$implode(string(), content_var.to_array());
  } else {
    content = content_var.to_string();
  }
  std::span<const char> data_span{content.c_str(), content.size()};

  if (flags & ~FILE_APPEND_FLAG) {
    kphp::log::warning("Flags other, than FILE_APPEND are not supported in file_put_contents");
    flags &= FILE_APPEND_FLAG;
  }

  const std::string_view* mode = &WRITE_MODE;
  if (flags & FILE_APPEND_FLAG) {
    mode = &APPEND_MODE;
  }
  if (auto sync_resource{
          from_mixed<class_instance<kphp::fs::sync_resource>>(f$fopen(stream, string{mode->data(), static_cast<string::size_type>(mode->size())}), {})};
      !sync_resource.is_null()) {
    auto expected{write_safe(sync_resource.get(), std::as_bytes(data_span))};
    return expected ? Optional<int64_t>{static_cast<int64_t>(*std::move(expected))} : Optional<int64_t>{false};
  }
  return false;
}

inline mixed f$getimagesize(const string& name) noexcept;
