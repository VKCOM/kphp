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
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <wchar.h>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-common/stdlib/array/array-functions.h"
#include "runtime-common/stdlib/string/string-functions.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/file/file-system-state.h"
#include "runtime-light/stdlib/file/resource.h"
#include "runtime-light/stdlib/fork/fork-functions.h"

namespace file_system_impl_ {

inline constexpr char SEPARATOR = '/';

} // namespace file_system_impl_

inline constexpr int32_t PHP_CSV_NO_ESCAPE = EOF;
inline constexpr size_t FGETS_BUFFER_SIZE = 128;
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
  if (auto stat_result{k2::stat({filename.c_str(), filename.size()}, std::addressof(stat))}; !stat_result.has_value()) [[unlikely]] {
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
    return expected ? resource{make_instance<kphp::fs::stdinput>(*std::move(expected))} : resource{false};
  } else if (filename_view == kphp::fs::STDOUT_NAME) {
    auto expected{kphp::fs::stdoutput::open()};
    return expected ? resource{make_instance<kphp::fs::stdoutput>(*std::move(expected))} : resource{false};
  } else if (filename_view == kphp::fs::STDERR_NAME) {
    auto expected{kphp::fs::stderror::open()};
    return expected ? resource{make_instance<kphp::fs::stderror>(*std::move(expected))} : resource{false};
  } else if (filename_view == kphp::fs::INPUT_NAME) {
    auto expected{kphp::fs::input::open()};
    return expected ? resource{make_instance<kphp::fs::input>(*std::move(expected))} : resource{false};
  } else if (filename_view.starts_with(kphp::fs::UDP_SCHEME_PREFIX) || filename_view.starts_with(kphp::fs::TCP_SCHEME_PREFIX)) {
    auto expected{kphp::fs::socket::open(filename_view)};
    return expected ? resource{make_instance<kphp::fs::socket>(*std::move(expected))} : resource{false};
  } else if (!filename_view.contains(kphp::fs::SCHEME_DELIMITER)) { // not a '*://*' pattern, so it must be a file
    auto expected{kphp::fs::file::open(filename_view, {mode.c_str(), mode.size()})};
    return expected ? resource{make_instance<kphp::fs::file>(*std::move(expected))} : resource{false};
  }

  kphp::log::warning("unexpected resource in fopen -> {}", filename.c_str());
  return resource{false};
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

inline bool f$file_exists(const string& name) noexcept {
  const auto exists_res{k2::access(std::string_view{name.c_str(), name.size()}, F_OK)};
  return exists_res.has_value();
}

inline bool f$unlink(const string& name) noexcept {
  std::expected<void, int32_t> unlink_res{k2::unlink(std::string_view{name.c_str(), name.size()})};
  return unlink_res.has_value();
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

inline Optional<string> f$file_get_contents(const string& stream) noexcept {
  if (auto sync_resource{from_mixed<class_instance<kphp::fs::sync_resource>>(f$fopen(stream, FileSystemImageState::get().READ_MODE), {})};
      !sync_resource.is_null()) {
    auto expected{sync_resource.get()->get_contents()};
    return expected ? Optional<string>{*std::move(expected)} : Optional<string>{false};
  }
  return false;
}

inline Optional<array<string>> f$file(const string& name) noexcept {
  struct stat stat_buf {};

  auto expected_file{kphp::fs::file::open(name.c_str(), "r")};
  if (!expected_file.has_value()) {
    return false;
  }
  if (!k2::stat({name.c_str(), name.size()}, std::addressof(stat_buf)).has_value()) {
    return false;
  }
  if (!S_ISREG(stat_buf.st_mode)) {
    kphp::log::warning("regular file expected as first argument in function file, \"{}\" is given", name.c_str());
    return false;
  }

  const size_t size{static_cast<size_t>(stat_buf.st_size)};
  if (size > string::max_size()) {
    kphp::log::warning("file \"{}\" is too large", name.c_str());
    return false;
  }

  kphp::stl::vector<std::byte, kphp::memory::script_allocator> file_content;
  file_content.resize(size);
  {
    auto file{std::move(*expected_file)};
    if (auto expected_read_result{file.read(file_content)}; !expected_read_result.has_value() || *expected_read_result < size) {
      return false;
    }
  }

  array<string> result;
  int32_t prev{-1};
  for (size_t i{0}; i < size; i++) {
    if (static_cast<char>(file_content[i]) == '\n' || i + 1 == size) {
      result.push_back(string{reinterpret_cast<char*>(file_content.data()) + prev + 1, static_cast<string::size_type>(i - prev)});
      prev = i;
    }
  }

  return result;
}

inline bool f$is_file(const string& name) noexcept {
  struct stat stat_buf {};
  // TODO: the semantics in PHP are different: PHP expects stat
  if (!k2::lstat(name.c_str(), std::addressof(stat_buf)).has_value()) {
    return false;
  }
  return S_ISREG(stat_buf.st_mode);
}

inline Optional<string> f$fgets(const resource& stream, int64_t length = -1) noexcept {
  auto sync_resource{from_mixed<class_instance<kphp::fs::file>>(stream, {})};
  if (sync_resource.is_null()) {
    return false;
  }
  kphp::fs::file* file{sync_resource.get()};
  if (file == nullptr) {
    return false;
  }
  if (struct stat stat_buf{}; !file->has_mmap_resource() && (!k2::fstat(file->descriptor(), std::addressof(stat_buf)).has_value() ||
                                                             !file->mmap(nullptr, stat_buf.st_size, PROT_READ, MAP_PRIVATE, 0).has_value())) {
    return false;
  }

  kphp::log::assertion(file->has_mmap_resource());
  char*& ptr{file->mmap_resource().value()->as_ptr()};

  if (*ptr == '\0' || length == 1) {
    return false;
  }

  if (length > string::max_size()) {
    kphp::log::warning("parameter length in function fgets is too large");
    return false;
  }

  if (length <= 0) {
    length = string::max_size();
  }
  --length;

  kphp::stl::string<kphp::memory::script_allocator> str{};
  std::array<char, FGETS_BUFFER_SIZE> buffer{};
  int64_t size{0};

  while (length > 0 && *ptr != '\0') {
    if (size == buffer.size()) {
      str.append(buffer.data(), size);
      size = 0;
    }
    buffer[size] = *ptr;
    ++size;
    if (*ptr == '\n') {
      ++ptr;
      break;
    }
    ++ptr;
    --length;
  }
  str.append(buffer.data(), size);
  return string{str.data(), static_cast<string::size_type>(str.size())};
}

[[clang::always_inline]] inline std::array<char, 2> read_last_two_bytes_advance(const char*& ptr, size_t len, mbstate_t* ps) noexcept {
  int32_t inc_len{};
  std::array<char, 2> last_chars{0, 0};
  while (len > 0) {
    inc_len = *ptr == '\0' ? 1 : mbrlen(ptr, len, ps);
    switch (inc_len) {
    case -2:
    case -1:
      inc_len = 1;
      break;
    case 0:
      return last_chars;
    case 1:
    default:
      last_chars[0] = last_chars[1];
      last_chars[1] = *ptr;
      break;
    }
    ptr += inc_len;
    len -= inc_len;
  }
  return last_chars;
}

// this function is imported from https://github.com/php/php-src/blob/master/ext/standard/file.c,
// function php_fgetcsv_lookup_trailing_spaces
inline const char* fgetcsv_lookup_trailing_spaces(const char* ptr, size_t len, mbstate_t* ps) noexcept {
  switch (const auto last_chars{read_last_two_bytes_advance(ptr, len, ps)}; last_chars[1]) {
  case '\n':
    if (last_chars[0] == '\r') {
      return ptr - 2;
    }
    /* fallthrough */
  case '\r':
    return ptr - 1;
  default:
    break;
  }
  return ptr;
}

[[clang::always_inline]] inline void advance_until_delimiter_mb(char const*& bptr, char const* line_end, int32_t& inc_len, const char delimiter,
                                                                mbstate_t* ps) noexcept {
  while (true) {
    switch (inc_len) {
    case 0:
      return;
    case -2:
    case -1:
      inc_len = 1;
      /* fallthrough */
    case 1:
      if (*bptr == delimiter) {
        return;
      }
      break;
    default:
      break;
    }
    bptr += inc_len;
    inc_len = bptr < line_end ? (*bptr == '\0' ? 1 : mbrlen(bptr, line_end - bptr, ps)) : 0;
  }
}

[[clang::always_inline]] inline bool parse_csv_quoted_field(const resource& stream, char const*& bptr, char const*& hunk_begin, char const*& buf,
                                                            std::span<const char>& line_end, string_buffer& tmp_buffer, std::span<const char>& buffer,
                                                            int32_t& inc_len, size_t& temp_len, const char enclosure, const char escape,
                                                            mbstate_t* ps) noexcept {
  int32_t state{0};
  while (true) {
    switch (inc_len) {
    case 0:
      switch (state) {
      case 2:
        tmp_buffer.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin - 1));
        hunk_begin = bptr;
        return false;

      case 1:
        tmp_buffer.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin));
        hunk_begin = bptr;
        /* fallthrough */
      case 0: {

        if (hunk_begin != line_end.data()) {
          tmp_buffer.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin));
          hunk_begin = bptr;
        }

        /* add the embedded line end to the field */
        tmp_buffer.append(line_end.data(), line_end.size());
        string new_buffer{};

        if (stream.is_null()) {
          return false;
        }
        Optional<string> new_buffer_optional{f$fgets(stream)};
        if (!new_buffer_optional.has_value()) {
          return temp_len <= static_cast<size_t>(line_end.data() - buf);
        }
        new_buffer = new_buffer_optional.val();
        temp_len += new_buffer.size();

        buffer = std::span<const char>{new_buffer.c_str(), static_cast<size_t>(new_buffer.size())};

        buf = bptr = buffer.data();
        hunk_begin = buf;

        const char* line_end_data{fgetcsv_lookup_trailing_spaces(buf, buffer.size(), ps)};
        line_end = std::span<const char>{line_end_data, buffer.size() - static_cast<size_t>(line_end_data - buf)};

        state = 0;
      } break;
      default:
        break;
      }
      break;

    case -2:
    case -1:
      /* break is omitted intentionally */
    case 1:
      /* we need to determine if the enclosure is
       * 'real' or is it escaped */
      switch (state) {
      case 1: /* escaped */
        bptr++;
        state = 0;
        break;
      case 2: /* embedded enclosure ? let's check it */
        if (*bptr != enclosure) {
          /* real enclosure */
          tmp_buffer.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin - 1));
          hunk_begin = bptr;
          return false;
        }
        tmp_buffer.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin));
        bptr++;
        hunk_begin = bptr;
        state = 0;
        break;
      default:
        if (*bptr == enclosure) {
          state = 2;
        } else if (escape != PHP_CSV_NO_ESCAPE && *bptr == escape) {
          state = 1;
        }
        bptr++;
        break;
      }
      break;

    default:
      switch (state) {
      case 2:
        /* real enclosure */
        tmp_buffer.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin - 1));
        hunk_begin = bptr;
        return false;
      case 1:
        bptr += inc_len;
        tmp_buffer.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin));
        hunk_begin = bptr;
        state = 0;
        break;
      default:
        bptr += inc_len;
        break;
      }
      break;
    }
    inc_len = bptr < line_end.data() ? (*bptr == '\0' ? 1 : mbrlen(bptr, line_end.data() - bptr, ps)) : 0;
  }
  return false;
}

// // Common csv-parsing functionality for
// // * fgetcsv
// // The function is similar to `php_fgetcsv` function from https://github.com/php/php-src/blob/master/ext/standard/file.c
inline Optional<array<mixed>> getcsv(const resource& stream, std::span<const char> buffer, const char delimiter, const char enclosure, const char escape,
                                     mbstate_t* ps) noexcept {
  array<mixed> answer{};
  int32_t current_id{0};
  string_buffer tmp_buffer{};

  const char* buf{buffer.data()};
  const char* bptr{buf};

  const char* line_end_data{fgetcsv_lookup_trailing_spaces(buf, buffer.size(), ps)};
  std::span<const char> line_end{line_end_data, buffer.size() - (line_end_data - buf)};

  bool first_field{true};
  size_t temp_len{buffer.size()};
  int32_t inc_len{};

  do {
    const char* hunk_begin{};

    inc_len = bptr < line_end.data() ? (*bptr == '\0' ? 1 : mbrlen(bptr, line_end.data() - bptr, ps)) : 0;
    if (inc_len == 1) {
      char const* tmp{bptr};
      while ((*tmp != delimiter) && isspace(static_cast<int32_t>(*tmp))) {
        tmp++;
      }
      if (*tmp == enclosure) {
        bptr = tmp;
      }
    }

    if (first_field && bptr == line_end.data()) {
      answer.set_value(current_id, mixed{});
      break;
    }
    first_field = false;

    /* 2. Read field, leaving bptr pointing at start of next field */
    if (inc_len != 0 && *bptr == enclosure) {

      bptr++; /* move on to first character in field */
      hunk_begin = bptr;

      /* 2A. handle enclosure delimited field */
      if (parse_csv_quoted_field(stream, bptr, hunk_begin, buf, line_end, tmp_buffer, buffer, inc_len, temp_len, enclosure, escape, ps)) {
        return answer;
      }
      /* look up for a delimiter */
      advance_until_delimiter_mb(bptr, line_end.data(), inc_len, delimiter, ps);
      tmp_buffer.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin));
      bptr += inc_len;
    } else {
      /* 2B. Handle non-enclosure field */

      hunk_begin = bptr;
      advance_until_delimiter_mb(bptr, line_end.data(), inc_len, delimiter, ps);
      tmp_buffer.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin));

      const char* comp_end{fgetcsv_lookup_trailing_spaces(tmp_buffer.c_str(), tmp_buffer.size(), ps)};
      tmp_buffer.set_pos(comp_end - tmp_buffer.c_str());
      if (*bptr == delimiter) {
        bptr++;
      }
    }

    /* 3. Now pass our field back to php */
    answer.set_value(current_id++, tmp_buffer.str());
    tmp_buffer.clean();
  } while (inc_len > 0);

  return answer;
}

inline Optional<array<mixed>> f$fgetcsv(const resource& stream, int64_t length = 0, string delimiter = string(",", 1), string enclosure = string("\"", 1),
                                        string escape = string("\\", 1)) noexcept {
  if (delimiter.empty()) {
    kphp::log::warning("delimiter must be a character");
    return false;
  }
  if (delimiter.size() > 1) {
    kphp::log::warning("delimiter must be a single character");
  }
  if (enclosure.empty()) {
    kphp::log::warning("enclosure must be a character");
    return false;
  }
  if (enclosure.size() > 1) {
    kphp::log::warning("enclosure must be a single character");
  }
  int escape_char{PHP_CSV_NO_ESCAPE};
  if (!escape.empty()) {
    escape_char = static_cast<int>(escape[0]);
  } else if (escape.size() > 1) {
    kphp::log::warning("escape_char must be a single character");
  }

  const char delimiter_char{delimiter[0]};
  const char enclosure_char{enclosure[0]};

  if (length < 0) {
    kphp::log::warning("length parameter may not be negative");
    return false;
  }
  if (length == 0) {
    length = -1;
  }
  Optional<string> buf_optional{length < 0 ? f$fgets(stream) : f$fgets(stream, length + 1)};
  if (!buf_optional.has_value()) {
    return false;
  }
  mbstate_t ps{};
  return getcsv(stream, std::span<const char>{buf_optional.val().c_str(), buf_optional.val().size()}, delimiter_char, enclosure_char, escape_char,
                std::addressof(ps));
}

inline Optional<int64_t> f$file_put_contents(const string& stream, const mixed& content_var, int64_t flags = 0) noexcept {
  string content{content_var.is_array() ? f$implode(string{}, content_var.to_array()) : content_var.to_string()};

  constexpr int64_t FILE_APPEND_FLAG{1};
  if (flags & ~FILE_APPEND_FLAG) {
    kphp::log::warning("flags other, than FILE_APPEND are not supported in file_put_contents");
    flags &= FILE_APPEND_FLAG;
  }

  const auto& file_system_lib_constants{FileSystemImageState::get()};
  const string& mode{((flags & FILE_APPEND_FLAG) != 0) ? file_system_lib_constants.APPEND_MODE : file_system_lib_constants.WRITE_MODE};
  if (auto sync_resource{from_mixed<class_instance<kphp::fs::sync_resource>>(f$fopen(stream, mode), {})}; !sync_resource.is_null()) {
    std::span<const char> data_span{content.c_str(), content.size()};
    auto expected{sync_resource.get()->write(std::as_bytes(data_span))};
    return expected ? Optional<int64_t>{static_cast<int64_t>(*std::move(expected))} : Optional<int64_t>{false};
  }
  return false;
}

mixed f$getimagesize(const string& name) noexcept;

// don't forget to add "interruptible" to file-functions.txt when this function becomes a coroutine
Optional<string> f$fgets(const resource& stream, int64_t length = -1) noexcept;
