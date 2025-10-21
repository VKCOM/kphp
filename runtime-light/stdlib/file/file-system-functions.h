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
#include <unistd.h>
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
  // TODO Why is empty `mode` passed here to f$fopen?
  if (auto sync_resource{from_mixed<class_instance<kphp::fs::sync_resource>>(f$fopen(stream, {}), {})}; !sync_resource.is_null()) {
    auto expected{sync_resource.get()->get_contents()};
    return expected ? Optional<string>{*std::move(expected)} : Optional<string>{false};
  }
  return false;
}

// TODO move write_safe() and read_safe() to utils.cpp or leave them here ??
inline std::expected<size_t, int32_t> write_safe(kphp::fs::sync_resource* resource, std::span<const std::byte> src) {
  /* TODO need tracing?
  if (kphp_tracing::is_turned_on()) {
    kphp_tracing::on_file_io_start(fd, file_name, true);
  }*/
  size_t full_len = src.size();
  do {
    std::expected<size_t, int32_t> cur_res = resource->write(src);
    if (!cur_res) {
      /*if (kphp_tracing::is_turned_on()) {
        kphp_tracing::on_file_io_fail(fd, -errno); // errno is positive, pass negative
      }*/
      return cur_res;
    }

    src = src.subspan(*cur_res);
  } while (!src.empty());
  /*if (kphp_tracing::is_turned_on()) {
    kphp_tracing::on_file_io_finish(fd, full_len - len);
  }*/

  // TODO need '- src.size()' ?
  return std::expected<size_t, int32_t>{full_len - src.size()};
}

inline std::expected<size_t, int32_t> read_safe(kphp::fs::sync_resource* resource, std::span<std::byte> dst) {
  /*TODO if (kphp_tracing::is_turned_on()) {
    kphp_tracing::on_file_io_start(fd, file_name, false);
  }*/
  size_t full_len = dst.size();
  do {
    std::expected<size_t, int32_t> cur_res = resource->read(dst);
    if (!cur_res) {
      /*if (kphp_tracing::is_turned_on()) {
        kphp_tracing::on_file_io_fail(fd, -errno); // errno is positive, pass negative
      }*/
      return cur_res;
    }
    // TODO need this ?
    if (*cur_res == 0) {
      break;
    }

    dst.subspan(*cur_res);
  } while (!dst.empty());
  /*if (kphp_tracing::is_turned_on()) {
    kphp_tracing::on_file_io_finish(fd, full_len - len);
  }*/

  // TODO need '- dst.size()' ?
  return full_len - dst.size();
}

constexpr int64_t FILE_APPEND_FLAG = 1;
const string READ_MODE = string(1, 'r');
const string WRITE_MODE = string(1, 'w');
const string APPEND_MODE = string(1, 'a');

inline Optional<int64_t> f$file_put_contents(const string& stream, const mixed& content_var, int64_t flags = 0) noexcept {
  string content;
  if (content_var.is_array()) {
    content = f$implode(string(), content_var.to_array());
  } else {
    content = content_var.to_string();
  }
  std::span<const char> data_span{content.c_str(), content.size()};

  if (flags & ~FILE_APPEND_FLAG) {
    // TODO
    php_warning("Flags other, than FILE_APPEND are not supported in file_put_contents");
    // TODO I don't understand why wrong flags forces FILE_APPEND to bet set in old runtime.
    flags &= FILE_APPEND_FLAG;
  }

  const string* mode = &WRITE_MODE;
  if (flags & FILE_APPEND_FLAG) {
    mode = &APPEND_MODE;
  }
  if (auto sync_resource{from_mixed<class_instance<kphp::fs::sync_resource>>(f$fopen(stream, *mode), {})}; !sync_resource.is_null()) {
    auto expected{write_safe(sync_resource.get(), std::as_bytes(data_span))};
    return expected ? Optional<int64_t>{static_cast<int64_t>(*std::move(expected))} : Optional<int64_t>{false};
  }
  return false;
}

// TODO think about const types
constexpr int64_t IMAGETYPE_UNKNOWN = 0;
constexpr int64_t IMAGETYPE_GIF = 1;
constexpr int64_t IMAGETYPE_JPEG = 2;
constexpr int64_t IMAGETYPE_PNG = 3;
constexpr int64_t IMAGETYPE_SWF = 4;
constexpr int64_t IMAGETYPE_PSD = 5;
constexpr int64_t IMAGETYPE_BMP = 6;
constexpr int64_t IMAGETYPE_TIFF_II = 7;
constexpr int64_t IMAGETYPE_TIFF_MM = 8;
constexpr int64_t IMAGETYPE_JPC = 9;
constexpr int64_t IMAGETYPE_JPEG2000 = 9;
constexpr int64_t IMAGETYPE_JP2 = 10;

// TODO think about const types
static const char php_sig_gif[3] = {'G', 'I', 'F'};
static const char php_sig_jpg[3] = {(char)0xff, (char)0xd8, (char)0xff};
static const char php_sig_png[8] = {(char)0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
static const char php_sig_jpc[4] = {(char)0xff, 0x4f, (char)0xff, 0x51};
static const char php_sig_jp2[12] = {0x00, 0x00, 0x00, 0x0c, 'j', 'P', ' ', ' ', 0x0d, 0x0a, (char)0x87, 0x0a};

static const char* mime_type_string[11] = {"",          "image/gif",      "image/jpeg", "image/png",  "application/x-shockwave-flash",
                                           "image/psd", "image/x-ms-bmp", "image/tiff", "image/tiff", "application/octet-stream",
                                           "image/jp2"};

// TODO think about const types
static const int M_SOF0 = 0xC0;
static const int M_SOF1 = 0xC1;
static const int M_SOF2 = 0xC2;
static const int M_SOF3 = 0xC3;
static const int M_SOF5 = 0xC5;
static const int M_SOF6 = 0xC6;
static const int M_SOF7 = 0xC7;
static const int M_SOF9 = 0xC9;
static const int M_SOF10 = 0xCA;
static const int M_SOF11 = 0xCB;
static const int M_SOF13 = 0xCD;
static const int M_SOF14 = 0xCE;
static const int M_SOF15 = 0xCF;
static const int M_EOI = 0xD9;
static const int M_SOS = 0xDA;
static const int M_COM = 0xFE;

static const int M_PSEUDO = 0xFFD8;

// TODO extract to separate .cpp file?
inline mixed f$getimagesize(const string& name) {
  // TODO implement k2_fstat, with fd as parameter !!!
  struct stat stat_buf;
  int32_t res = k2_stat(name.c_str(), name.size(), &stat_buf);
  if (res != k2::errno_ok) {
    return false;
  }

  auto sync_res{from_mixed<class_instance<kphp::fs::sync_resource>>(f$fopen(name, READ_MODE), {})};
  if (sync_res.is_null()) {
    // TODO what is fucking false?
    return false;
  }
  auto f = sync_res.get();

  if (!S_ISREG(stat_buf.st_mode)) {
    php_warning("Regular file expected as first argument in function getimagesize, \"%s\" is given", name.c_str());
    return false;
  }

  const size_t min_size = 3 * 256 + 64;
  unsigned char buf[min_size];
  size_t size = stat_buf.st_size, read_size = min_size;
  if (size < min_size) {
    read_size = size;
  }
  std::span<unsigned char> buf_span(buf, min_size);

  if (read_size < 12) {
    return false;
  }
  std::expected<size_t, int32_t> read_res = read_safe(f, std::as_writable_bytes(buf_span));
  if (!read_res || *read_res < read_size) {
    return false;
  }

  int width = 0, height = 0, bits = 0, channels = 0, type = IMAGETYPE_UNKNOWN;
  switch (buf[0]) {
  case 'G': // gif
    if (!strncmp((const char*)buf, php_sig_gif, sizeof(php_sig_gif))) {
      type = IMAGETYPE_GIF;
      width = buf[6] | (buf[7] << 8);
      height = buf[8] | (buf[9] << 8);
      bits = buf[10] & 0x80 ? (buf[10] & 0x07) + 1 : 0;
      channels = 3;
    } else {
      return false;
    }
    break;
  case 0xff: // jpg or jpc
    if (!strncmp((const char*)buf, php_sig_jpg, sizeof(php_sig_jpg))) {
      type = IMAGETYPE_JPEG;

      // TODO what allocator to use? script allocator or k2_alloc? + what is the difference between alloc and alloc0?
      unsigned char* image = (unsigned char*)RuntimeAllocator::get().alloc_script_memory(size);
      if (image == nullptr) {
        php_warning("Not enough memory to process file \"%s\" in getimagesize", name.c_str());
        return false;
      }
      memcpy(image, buf, read_size);

      std::span<unsigned char> image_span(image + read_size, size - read_size);
      read_res = read_safe(f, std::as_writable_bytes(image_span));
      if (!read_res || *read_res < size - read_size) {
        // TODO need cast from unsigned char* to void* ?
        RuntimeAllocator::get().free_script_memory(image, size);
        return false;
      }

      int marker = M_PSEUDO;
      size_t cur_pos = 2;

      while (height == 0 && width == 0 && marker != M_SOS && marker != M_EOI) {
        int a = 0, comment_correction = 1 + (marker == M_COM), new_marker;

        do {
          if (cur_pos == size) {
            new_marker = M_EOI;
            break;
          }
          new_marker = image[cur_pos++];
          if (marker == M_COM && comment_correction > 0) {
            if (new_marker != 0xFF) {
              new_marker = 0xFF;
              comment_correction--;
            } else {
              marker = M_PSEUDO;
            }
          }
          a++;
        } while (new_marker == 0xff);

        if (a < 2 || (marker == M_COM && comment_correction)) {
          new_marker = M_EOI;
        }

        marker = new_marker;

        switch (marker) {
        case M_SOF0:
        case M_SOF1:
        case M_SOF2:
        case M_SOF3:
        case M_SOF5:
        case M_SOF6:
        case M_SOF7:
        case M_SOF9:
        case M_SOF10:
        case M_SOF11:
        case M_SOF13:
        case M_SOF14:
        case M_SOF15:
          if (cur_pos + 8 > size) {
            RuntimeAllocator::get().free_script_memory(image, size);
            return false;
          }
          bits = image[cur_pos + 2];
          height = (image[cur_pos + 3] << 8) + image[cur_pos + 4];
          width = (image[cur_pos + 5] << 8) + image[cur_pos + 6];
          channels = image[cur_pos + 7];
          cur_pos += 8;

        case M_SOS:
        case M_EOI:
          break;

        default: {
          size_t length = (image[cur_pos] << 8) + image[cur_pos + 1];

          if (length < 2 || cur_pos + length > size) {
            RuntimeAllocator::get().free_script_memory(image, size);
            return false;
          }
          cur_pos += length;
          break;
        }
        }
      }
      RuntimeAllocator::get().free_script_memory(image, size);
    } else if (!strncmp((const char*)buf, php_sig_jpc, sizeof(php_sig_jpc)) && static_cast<int>(read_size) >= 42) {
      type = IMAGETYPE_JPEG;

      width = (buf[8] << 24) + (buf[9] << 16) + (buf[10] << 8) + buf[11];
      height = (buf[12] << 24) + (buf[13] << 16) + (buf[14] << 8) + buf[15];
      channels = (buf[40] << 8) + buf[41];

      if (channels < 0 || channels > 256 || static_cast<int>(read_size) < 42 + 3 * channels || width <= 0 || height <= 0) {
        return false;
      }

      bits = 0;
      for (int i = 0; i < channels; i++) {
        int cur_bits = buf[42 + 3 * i];
        if (cur_bits > bits) {
          bits = cur_bits;
        }
      }
      bits++;
    } else {
      return false;
    }
    break;
  case 0x00: // jp2
    if (read_size >= 54 && !strncmp((const char*)buf, php_sig_jp2, sizeof(php_sig_jp2))) {
      type = IMAGETYPE_JP2;

      bool found = false;

      int buf_pos = 12;
      size_t file_pos = 12;
      while (static_cast<int>(read_size) >= 42 + buf_pos + 8) {
        const unsigned char* s = buf + buf_pos;
        int box_length = (s[0] << 24) + (s[1] << 16) + (s[2] << 8) + s[3];
        if (box_length == 1 || box_length > 1000000000) {
          break;
        }
        if (s[4] == 'j' && s[5] == 'p' && s[6] == '2' && s[7] == 'c') {
          s += 8;

          width = (s[8] << 24) + (s[9] << 16) + (s[10] << 8) + s[11];
          height = (s[12] << 24) + (s[13] << 16) + (s[14] << 8) + s[15];
          channels = (s[40] << 8) + s[41];

          if (channels < 0 || channels > 256 || static_cast<int>(read_size) < 42 + buf_pos + 8 + 3 * channels || width <= 0 || height <= 0) {
            break;
          }

          bits = 0;
          for (int i = 0; i < channels; i++) {
            int cur_bits = s[42 + 3 * i];
            if (cur_bits > bits) {
              bits = cur_bits;
            }
          }
          bits++;

          found = true;
          break;
        }

        if (box_length <= 8) {
          break;
        }
        file_pos += box_length;
        if (file_pos >= size || static_cast<off_t>(file_pos) != static_cast<ssize_t>(file_pos) || static_cast<ssize_t>(file_pos) < 0) {
          break;
        }

        read_size = min_size;
        if (size - file_pos < min_size) {
          read_size = size - file_pos;
        }

        if (read_size < 50) {
          break;
        }
        std::span<unsigned char> buf_read_span(buf, read_size);
        read_res = f->pread(std::as_writable_bytes(buf_read_span), static_cast<off_t>(file_pos));
        if (!read_res || *read_res < read_size) {
          break;
        }

        buf_pos = 0;
      }

      if (!found) {
        return false;
      }
    } else {
      return false;
    }
    break;
  case 0x89: // png
    if (read_size >= 25 && !strncmp((const char*)buf, php_sig_png, sizeof(php_sig_png))) {
      type = IMAGETYPE_PNG;
      width = (buf[16] << 24) + (buf[17] << 16) + (buf[18] << 8) + buf[19];
      height = (buf[20] << 24) + (buf[21] << 16) + (buf[22] << 8) + buf[23];
      bits = buf[24];
    } else {
      return false;
    }
    break;
  default:
    return false;
  }

  // TODO what allocator is used in these structures (array<mixed>, string) ???
  array<mixed> result(array_size(7, false));
  result.push_back(width);
  result.push_back(height);
  result.push_back(type);

  // TODO is snpritnf ok?
  int len = snprintf(reinterpret_cast<char*>(buf), min_size, "width=\"%d\" height=\"%d\"", width, height);
  result.push_back(string((const char*)buf, len));
  if (bits != 0) {
    result.set_value(string("bits", 4), bits);
  }
  if (channels != 0) {
    result.set_value(string("channels", 8), channels);
  }
  result.set_value(string("mime", 4), string(mime_type_string[type]));

  return result;
}
