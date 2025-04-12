// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string_view>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/file/file-system-state.h"
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

resource f$fopen(const string& filename, const string& mode, bool use_include_path = false, const resource& context = {}) noexcept;

kphp::coro::task<Optional<int64_t>> f$fwrite(resource stream, string text) noexcept;

bool f$fflush(const resource& stream) noexcept;

bool f$fclose(const resource& stream) noexcept;

resource f$stream_socket_client(const string& address, mixed& error_code = FileSystemInstanceState::get().error_number_dummy,
                                mixed& error_message = FileSystemInstanceState::get().error_description_dummy, double timeout = DEFAULT_SOCKET_TIMEOUT,
                                int64_t flags = STREAM_CLIENT_CONNECT, const resource& context = {}) noexcept;

Optional<string> f$file_get_contents(const string& stream) noexcept;
