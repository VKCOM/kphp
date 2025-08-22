// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <sys/stat.h>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/file/resource.h"

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

inline resource f$fopen(const string& filename, [[maybe_unused]] const string& mode, [[maybe_unused]] bool use_include_path = false,
                        [[maybe_unused]] const resource& context = {}) noexcept {
  kphp::resource::underlying_resource rsrc{{filename.c_str(), filename.size()}};
  if (rsrc.error_code() != k2::errno_ok) [[unlikely]] {
    kphp::log::warning("fopen failed: {}", filename.c_str());
    return {};
  }

  return f$to_mixed(make_instance<kphp::resource::underlying_resource>(std::move(rsrc)));
}

inline bool f$fclose(const resource& stream) noexcept {
  auto rsrc{from_mixed<class_instance<kphp::resource::underlying_resource>>(stream, {})};
  if (rsrc.is_null()) [[unlikely]] {
    kphp::log::warning("unexpected resource in fclose: {}", stream.to_string().c_str());
    return false;
  }

  rsrc.get()->close();
  return true;
}

inline kphp::coro::task<Optional<int64_t>> f$fwrite(resource stream, string text) noexcept {
  auto rsrc{from_mixed<class_instance<kphp::resource::underlying_resource>>(stream, {})};
  if (rsrc.is_null()) [[unlikely]] {
    kphp::log::warning("unexpected resource in fwrite: {}", stream.to_string().c_str());
    co_return false;
  }
  co_return co_await rsrc.get()->write({text.c_str(), text.size()});
}

inline bool f$fflush(const resource& stream) noexcept {
  auto rsrc{from_mixed<class_instance<kphp::resource::underlying_resource>>(stream, {})};
  if (rsrc.is_null()) [[unlikely]] {
    kphp::log::warning("unexpected resource in fflush: {}", stream.to_string().c_str());
    return false;
  }

  rsrc.get()->flush();
  return true;
}

inline resource f$stream_socket_client(const string& address, std::optional<std::reference_wrapper<mixed>> error_code = {},
                                       std::optional<std::reference_wrapper<mixed>> /*error_message*/ = {}, double /*timeout*/ = DEFAULT_SOCKET_TIMEOUT,
                                       int64_t /*flags*/ = STREAM_CLIENT_CONNECT, const resource& /*context*/ = {}) noexcept {
  /*
   * TODO: Here should be waiting with timeout,
   *       but it can't be expressed simple ways by awaitables since we blocked inside k2
   * */
  const std::string_view address_view{address.c_str(), address.size()};
  const auto address_kind{kphp::resource::uri_to_resource_kind(address_view)};
  if (address_kind != kphp::resource::resource_kind::UDP) [[unlikely]] {
    return static_cast<int64_t>(k2::errno_einval);
  }

  kphp::resource::underlying_resource rsrc{address_view};
  if (rsrc.error_code() != k2::errno_ok && error_code.has_value()) [[unlikely]] {
    (*error_code).get() = static_cast<int64_t>(rsrc.error_code());
    return {};
  }
  return f$to_mixed(make_instance<kphp::resource::underlying_resource>(std::move(rsrc)));
}

inline Optional<string> f$file_get_contents(const string& stream) noexcept {
  kphp::resource::underlying_resource rsrc{{stream.c_str(), stream.size()}};
  if (rsrc.error_code() != k2::errno_ok) [[unlikely]] {
    return false;
  }
  return rsrc.get_contents();
}
