// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/file/file-system-functions.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <format>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <sys/stat.h>
#include <utility>
#include <wchar.h>

#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/file/file-system-state.h"
#include "runtime-light/stdlib/file/resource.h"

namespace {

constexpr int32_t PHP_CSV_NO_ESCAPE = EOF;
constexpr size_t MIN_FILE_SIZE{12};
constexpr size_t MAX_READ_SIZE{3 * 256 + 64};

// constexpr int64_t IMAGETYPE_UNKNOWN{0};
constexpr int64_t IMAGETYPE_GIF{1};
constexpr int64_t IMAGETYPE_JPEG{2};
constexpr int64_t IMAGETYPE_PNG{3};
// constexpr int64_t IMAGETYPE_SWF{4};
// constexpr int64_t IMAGETYPE_PSD{5};
// constexpr int64_t IMAGETYPE_BMP{6};
// constexpr int64_t IMAGETYPE_TIFF_II{7};
// constexpr int64_t IMAGETYPE_TIFF_MM{8};
// constexpr int64_t IMAGETYPE_JPC{9};
// constexpr int64_t IMAGETYPE_JPEG2000 {9};
constexpr int64_t IMAGETYPE_JP2{10};

constexpr std::array<char, 3> php_sig_gif{'G', 'I', 'F'};
constexpr std::array<char, 3> php_sig_jpg{static_cast<char>(0xff), static_cast<char>(0xd8), static_cast<char>(0xff)};
constexpr std::array<char, 8> php_sig_png{static_cast<char>(0x89), 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
constexpr std::array<char, 4> php_sig_jpc{static_cast<char>(0xff), 0x4f, static_cast<char>(0xff), 0x51};
constexpr std::array<char, 12> php_sig_jp2{0x00, 0x00, 0x00, 0x0c, 'j', 'P', ' ', ' ', 0x0d, 0x0a, static_cast<char>(0x87), 0x0a};

constexpr std::array<std::string_view, 11> mime_type_string{"",          "image/gif",      "image/jpeg", "image/png",  "application/x-shockwave-flash",
                                                            "image/psd", "image/x-ms-bmp", "image/tiff", "image/tiff", "application/octet-stream",
                                                            "image/jp2"};

constexpr unsigned char GIF_MARKER{'G'};
constexpr unsigned char JPG_OR_JPC_MARKER{0xff};
constexpr unsigned char JP2_MARKER{0x00};
constexpr unsigned char PNG_MARKER{0x89};

constexpr int32_t M_SOF0{0xC0};
constexpr int32_t M_SOF1{0xC1};
constexpr int32_t M_SOF2{0xC2};
constexpr int32_t M_SOF3{0xC3};
constexpr int32_t M_SOF5{0xC5};
constexpr int32_t M_SOF6{0xC6};
constexpr int32_t M_SOF7{0xC7};
constexpr int32_t M_SOF9{0xC9};
constexpr int32_t M_SOF10{0xCA};
constexpr int32_t M_SOF11{0xCB};
constexpr int32_t M_SOF13{0xCD};
constexpr int32_t M_SOF14{0xCE};
constexpr int32_t M_SOF15{0xCF};
constexpr int32_t M_EOI{0xD9};
constexpr int32_t M_SOS{0xDA};
constexpr int32_t M_COM{0xFE};

constexpr int32_t M_PSEUDO = 0xFFD8;

struct image_info {
  int32_t type{};
  int32_t width{};
  int32_t height{};
  int32_t bits{};
  int32_t channels{};
};

std::optional<image_info> get_gif_info(std::span<unsigned char> buf) noexcept {
  if (std::strncmp(reinterpret_cast<const char*>(buf.data()), php_sig_gif.begin(), sizeof(php_sig_gif)) != 0) {
    return std::nullopt;
  }
  return {{
      .type = IMAGETYPE_GIF,
      .width = buf[6] | (buf[7] << 8),
      .height = buf[8] | (buf[9] << 8),
      .bits = buf[10] & 0x80 ? (buf[10] & 0x07) + 1 : 0,
      .channels = 3,
  }};
}

std::optional<image_info> get_jpg_info(uint64_t file_size, size_t read_size, std::span<unsigned char> buf, const string& name, kphp::fs::file file) noexcept {
  if (file_size > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
    kphp::log::warning("size of jpg file \"{}\" is more, than {}", name.c_str(), std::numeric_limits<size_t>::max());
    return std::nullopt;
  }

  image_info info{.type = IMAGETYPE_JPEG};

  std::unique_ptr<unsigned char, decltype(std::addressof(kphp::memory::script::free))> image_uniq_ptr{
      static_cast<unsigned char*>(kphp::memory::script::alloc(static_cast<size_t>(file_size))), kphp::memory::script::free};
  auto* image{image_uniq_ptr.get()};
  if (image == nullptr) {
    kphp::log::warning("not enough memory to process file \"{}\" in getimagesize", name.c_str());
    return std::nullopt;
  }

  std::memcpy(image, buf.data(), read_size);
  std::span<std::byte> image_span{std::next(reinterpret_cast<std::byte*>(image), read_size), static_cast<size_t>(file_size) - read_size};
  std::expected<size_t, int32_t> read_res{file.read(std::as_writable_bytes(image_span))};
  if (!read_res || *read_res < static_cast<size_t>(file_size) - read_size) {
    return std::nullopt;
  }

  int32_t marker{M_PSEUDO};
  size_t cur_pos{2};

  while (info.height == 0 && info.width == 0 && marker != M_SOS && marker != M_EOI) {
    int32_t a{};
    int32_t comment_correction{1 + (marker == M_COM)};
    int32_t new_marker{};

    do {
      if (cur_pos == static_cast<size_t>(file_size)) {
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
      if (cur_pos + 8 > static_cast<size_t>(file_size)) {
        return std::nullopt;
      }
      info.bits = image[cur_pos + 2];
      info.height = (image[cur_pos + 3] << 8) + image[cur_pos + 4];
      info.width = (image[cur_pos + 5] << 8) + image[cur_pos + 6];
      info.channels = image[cur_pos + 7];
      cur_pos += 8;

    case M_SOS:
    case M_EOI:
      break;

    default:
      size_t length{static_cast<size_t>((image[cur_pos] << 8) + image[cur_pos + 1])};

      if (length < 2 || cur_pos + length > static_cast<size_t>(file_size)) {
        return std::nullopt;
      }
      cur_pos += length;
      break;
    }
  }

  return {info};
}

std::optional<image_info> get_jpc_info(size_t read_size, std::span<unsigned char> buf) noexcept {
  image_info info{
      .type = IMAGETYPE_JPEG,
      .width = (buf[8] << 24) + (buf[9] << 16) + (buf[10] << 8) + buf[11],
      .height = (buf[12] << 24) + (buf[13] << 16) + (buf[14] << 8) + buf[15],
      .channels = (buf[40] << 8) + buf[41],
  };

  if (info.channels < 0 || info.channels > 256 || read_size < 42 + 3 * info.channels || info.width <= 0 || info.height <= 0) {
    return std::nullopt;
  }

  for (int32_t i{0}; i < info.channels; i++) {
    int32_t cur_bits{buf[42 + 3 * i]};
    if (cur_bits > info.bits) {
      info.bits = cur_bits;
    }
  }
  info.bits++;

  return {info};
}

std::optional<image_info> get_jpg_or_jpc_info(uint64_t file_size, size_t read_size, std::span<unsigned char> buf, const string& name,
                                              kphp::fs::file file) noexcept {
  if (!std::strncmp(reinterpret_cast<const char*>(buf.data()), php_sig_jpg.begin(), sizeof(php_sig_jpg))) {
    return get_jpg_info(file_size, read_size, buf, name, std::move(file));
  } else if (read_size >= 42 && !std::strncmp(reinterpret_cast<const char*>(buf.data()), php_sig_jpc.begin(), sizeof(php_sig_jpc))) {
    return get_jpc_info(read_size, buf);
  } else {
    return std::nullopt;
  }
}

std::optional<image_info> get_jp2_info(uint64_t file_size, size_t read_size, std::span<unsigned char> buf, kphp::fs::file file) noexcept {
  if (read_size < 54 || std::strncmp(reinterpret_cast<const char*>(buf.data()), php_sig_jp2.begin(), sizeof(php_sig_jp2)) != 0) {
    return std::nullopt;
  }

  image_info info{
      .type = IMAGETYPE_JP2,
  };

  bool found{false};
  int32_t buf_pos{12};
  uint64_t file_pos{12};
  while (static_cast<int32_t>(read_size) >= 42 + buf_pos + 8) {
    const unsigned char* s{std::next(buf.data(), buf_pos)};
    int32_t box_length{(s[0] << 24) + (s[1] << 16) + (s[2] << 8) + s[3]};
    if (box_length == 1 || box_length > 1000000000) {
      break;
    }
    if (s[4] == 'j' && s[5] == 'p' && s[6] == '2' && s[7] == 'c') {
      s += 8;

      info.width = (s[8] << 24) + (s[9] << 16) + (s[10] << 8) + s[11];
      info.height = (s[12] << 24) + (s[13] << 16) + (s[14] << 8) + s[15];
      info.channels = (s[40] << 8) + s[41];

      if (info.channels < 0 || info.channels > 256 || static_cast<int32_t>(read_size) < 42 + buf_pos + 8 + 3 * info.channels || info.width <= 0 ||
          info.height <= 0) {
        break;
      }

      for (int32_t i{0}; i < info.channels; i++) {
        int32_t cur_bits{s[42 + 3 * i]};
        if (cur_bits > info.bits) {
          info.bits = cur_bits;
        }
      }
      info.bits++;

      found = true;
      break;
    }

    if (box_length <= 8) {
      break;
    }
    file_pos += box_length;
    if (file_pos >= file_size) {
      break;
    }

    read_size = MAX_READ_SIZE;
    if (file_size - file_pos < static_cast<uint64_t>(MAX_READ_SIZE)) {
      read_size = static_cast<size_t>(file_size - file_pos);
    }
    if (read_size < 50) {
      break;
    }
    std::span<std::byte> buf_read_span{reinterpret_cast<std::byte*>(buf.data()), read_size};
    std::expected<size_t, int32_t> read_res{file.pread(std::as_writable_bytes(buf_read_span), file_pos)};
    if (!read_res || *read_res < read_size) {
      break;
    }

    buf_pos = 0;
  }

  if (!found) {
    return std::nullopt;
  }
  return {info};
}

std::optional<image_info> get_png_info(size_t read_size, std::span<unsigned char> buf) noexcept {
  if (read_size < 25 || std::strncmp(reinterpret_cast<const char*>(buf.data()), php_sig_png.begin(), sizeof(php_sig_png)) != 0) {
    return std::nullopt;
  }
  return {{
      .type = IMAGETYPE_PNG,
      .width = (buf[16] << 24) + (buf[17] << 16) + (buf[18] << 8) + buf[19],
      .height = (buf[20] << 24) + (buf[21] << 16) + (buf[22] << 8) + buf[23],
      .bits = buf[24],
  }};
}

} // namespace

mixed f$getimagesize(const string& name) noexcept {
  // TODO implement k2::fstat, with fd as parameter
  struct stat stat_buf {};
  if (!k2::stat({name.c_str(), name.size()}, std::addressof(stat_buf)).has_value()) {
    return false;
  }

  if (!S_ISREG(stat_buf.st_mode)) {
    kphp::log::warning("regular file expected as first argument in function getimagesize, \"{}\" is given", name.c_str());
    return false;
  }
  uint64_t file_size{static_cast<uint64_t>(stat_buf.st_size)};
  if (file_size < static_cast<uint64_t>(MIN_FILE_SIZE)) {
    return false;
  }

  const string& read_mode{FileSystemImageState::get().READ_MODE};
  auto open_res{kphp::fs::file::open({name.c_str(), name.size()}, {read_mode.c_str(), read_mode.size()})};
  if (!open_res) {
    return false;
  }
  auto file{std::move(*open_res)};

  size_t read_size{MAX_READ_SIZE};
  if (file_size < static_cast<uint64_t>(MAX_READ_SIZE)) {
    read_size = static_cast<size_t>(file_size);
  }
  std::array<unsigned char, MAX_READ_SIZE> buf{};
  std::span<std::byte> buf_span{reinterpret_cast<std::byte*>(buf.begin()), MAX_READ_SIZE};

  std::expected<size_t, int32_t> read_res{file.read(std::as_writable_bytes(buf_span))};
  if (!read_res || *read_res < read_size) {
    return false;
  }

  std::optional<image_info> info_opt{std::nullopt};
  switch (buf[0]) {
  case GIF_MARKER: // gif
    info_opt = get_gif_info(buf);
    break;
  case JPG_OR_JPC_MARKER: // jpg or jpc
    info_opt = get_jpg_or_jpc_info(file_size, read_size, buf, name, std::move(file));
    break;
  case JP2_MARKER: // jp2
    info_opt = get_jp2_info(file_size, read_size, buf, std::move(file));
    break;
  case PNG_MARKER: // png
    info_opt = get_png_info(read_size, buf);
    break;
  }

  if (!info_opt) {
    return false;
  }

  array<mixed> result{array_size(7, false)};
  result.push_back(info_opt->width);
  result.push_back(info_opt->height);
  result.push_back(info_opt->type);

  string::size_type len{static_cast<string::size_type>(
      std::distance(reinterpret_cast<char*>(buf.data()),
                    std::format_to_n(reinterpret_cast<char*>(buf.data()), MAX_READ_SIZE, R"(width="{}" height="{}")", info_opt->width, info_opt->height).out))};
  result.push_back(string{reinterpret_cast<const char*>(buf.begin()), len});

  if (info_opt->bits != 0) {
    result.set_value(string{"bits", 4}, info_opt->bits);
  }
  if (info_opt->channels != 0) {
    result.set_value(string{"channels", 8}, info_opt->channels);
  }
  result.set_value(string{"mime", 4}, string{mime_type_string[info_opt->type].data(), static_cast<string::size_type>(mime_type_string[info_opt->type].size())});

  return result;
}

// don't forget to add "interruptible" to file-functions.txt when this function becomes a coroutine
Optional<string> f$fgets(const resource& stream, int64_t length) noexcept {
  if (length == 0) {
    return false;
  }

  if (length == 1) {
    return string{};
  }

  auto file_resource{from_mixed<class_instance<kphp::fs::file>>(stream, {})};
  if (file_resource.is_null()) {
    return false;
  }
  kphp::log::assertion(file_resource.get() != nullptr);
  const kphp::fs::file& file{*file_resource.get()};

  if (length < 0) {
    struct stat st {};
    kphp::log::assertion(k2::fstat(file.descriptor(), std::addressof(st)).has_value());
    if (st.st_size <= 0) {
      return false;
    }
    length = st.st_size + 1;
  }

  if (length > string::max_size()) {
    kphp::log::warning("parameter length in function fgets mustn't be greater than string::max_size(). length = {}, string::max_size() = {}", length,
                       string::max_size());
    return false;
  }
  string res{static_cast<string::size_type>(length), false};
  auto read_res{k2::readline(file.descriptor(), std::as_writable_bytes(std::span<char>{res.buffer(), res.size()}))};

  if (read_res == 0) {
    return false;
  }
  res.shrink(static_cast<string::size_type>(read_res));
  return res;
}

namespace {
[[clang::always_inline]] std::pair<char, char> read_last_two_bytes_advance(const char*& ptr, size_t len, mbstate_t* ps) noexcept {
  int32_t inc_len{};
  std::pair<char, char> last_chars{0, 0};
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
      last_chars.first = last_chars.second;
      last_chars.second = *ptr;
      break;
    }
    ptr += inc_len;
    len -= inc_len;
  }
  return last_chars;
}

// this function is imported from https://github.com/php/php-src/blob/master/ext/standard/file.c,
// function php_fgetcsv_lookup_trailing_spaces
const char* fgetcsv_lookup_trailing_spaces(const char* ptr, size_t len, mbstate_t* ps) noexcept {
  switch (const auto last_chars{read_last_two_bytes_advance(ptr, len, ps)}; last_chars.second) {
  case '\n':
    if (last_chars.first == '\r') {
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

[[clang::always_inline]] void advance_until_delimiter_mb(const char*& bptr, const char* line_end, int32_t& inc_len, const char delimiter,
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

[[clang::always_inline]] bool parse_csv_quoted_field(const resource& stream, const char*& bptr, const char*& hunk_begin, std::span<const char>& line_end,
                                                     string_buffer& field_buf, string& line, int32_t& inc_len, size_t& temp_len, const char enclosure,
                                                     const char escape, mbstate_t* ps) noexcept {
  enum class State { NormalProcessing, EscapeProcessing, EnclosureProcessing };
  auto state{State::NormalProcessing};

  while (true) {
    switch (inc_len) {
    case 0: // end of line
      switch (state) {
      case State::EnclosureProcessing:
        field_buf.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin - 1));
        hunk_begin = bptr;
        return false;

      case State::EscapeProcessing:
        field_buf.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin));
        hunk_begin = bptr;
        /* fallthrough */
      case State::NormalProcessing:

        if (hunk_begin != line_end.data()) {
          field_buf.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin));
          hunk_begin = bptr;
        }

        /* add the embedded line end to the field */
        field_buf.append(line_end.data(), line_end.size());

        if (stream.is_null()) {
          return false;
        }

        Optional<string> new_buffer_optional{f$fgets(stream)};
        if (!new_buffer_optional.has_value()) {
          return temp_len <= static_cast<size_t>(line_end.data() - line.c_str());
        }

        line = new_buffer_optional.val();
        temp_len += line.size();

        bptr = line.c_str();
        hunk_begin = line.c_str();

        const char* line_end_data{fgetcsv_lookup_trailing_spaces(line.c_str(), line.size(), ps)};
        line_end = std::span<const char>{line_end_data, line.size() - static_cast<size_t>(line_end_data - line.c_str())};

        state = State::NormalProcessing;
        break;
      }
      break;

    case -2:
    case -1:
      /* break is omitted intentionally */
    case 1: // ascii
      /* we need to determine if the enclosure is
       * 'real' or is it escaped */
      switch (state) {
      case State::EscapeProcessing: /* escaped */
        bptr++;
        state = State::NormalProcessing;
        break;
      case State::EnclosureProcessing: /* embedded enclosure ? let's check it */
        if (*bptr != enclosure) {
          /* real enclosure */
          field_buf.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin - 1));
          hunk_begin = bptr;
          return false;
        }
        field_buf.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin));
        bptr++;
        hunk_begin = bptr;
        state = State::NormalProcessing;
        break;
      case State::NormalProcessing:
        if (*bptr == enclosure) {
          state = State::EnclosureProcessing;
        } else if (escape != PHP_CSV_NO_ESCAPE && *bptr == escape) {
          state = State::EscapeProcessing;
        }
        bptr++;
        break;
      }
      break;

    default: // not ascii
      switch (state) {
      case State::EnclosureProcessing:
        /* real enclosure */
        field_buf.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin - 1));
        hunk_begin = bptr;
        return false;
      case State::EscapeProcessing:
        bptr += inc_len;
        field_buf.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin));
        hunk_begin = bptr;
        state = State::NormalProcessing;
        break;
      case State::NormalProcessing:
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
Optional<array<mixed>> getcsv(const resource& stream, string line, const char delimiter, const char enclosure, const char escape, mbstate_t* ps) noexcept {
  array<mixed> answer{};
  int32_t current_id{0};
  string_buffer field_buf{};

  const char* bptr{line.c_str()};

  const char* line_end_data{fgetcsv_lookup_trailing_spaces(line.c_str(), line.size(), ps)};
  std::span<const char> line_end{line_end_data, static_cast<size_t>(line.size() - (line_end_data - line.c_str()))};

  bool first_field{true};
  size_t temp_len{line.size()};
  int32_t inc_len{};

  do {
    const char* hunk_begin{};

    inc_len = bptr < line_end.data() ? (*bptr == '\0' ? 1 : mbrlen(bptr, line_end.data() - bptr, ps)) : 0;
    if (inc_len == 1) {
      const char* tmp{bptr};
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
      if (parse_csv_quoted_field(stream, bptr, hunk_begin, line_end, field_buf, line, inc_len, temp_len, enclosure, escape, ps)) {
        return answer;
      }
      /* look up for a delimiter */
      advance_until_delimiter_mb(bptr, line_end.data(), inc_len, delimiter, ps);
      field_buf.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin));
      bptr += inc_len; // inc_len = 0(eol)|1(delimiter)
    } else {
      /* 2B. Handle non-enclosure field */

      hunk_begin = bptr;
      advance_until_delimiter_mb(bptr, line_end.data(), inc_len, delimiter, ps);
      field_buf.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin));

      const char* comp_end{fgetcsv_lookup_trailing_spaces(field_buf.c_str(), field_buf.size(), ps)};
      field_buf.set_pos(comp_end - field_buf.c_str());
      if (*bptr == delimiter) {
        bptr++;
      }
    }

    /* 3. Now pass our field back to php */
    answer.set_value(current_id++, field_buf.str());
    field_buf.clean();
  } while (inc_len > 0);

  return answer;
}
} // namespace

// don't forget to add "interruptible" to file-functions.txt when this function becomes a coroutine
Optional<array<mixed>> f$fgetcsv(const resource& stream, int64_t length, string delimiter, string enclosure, string escape) noexcept {
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
  int32_t escape_char{PHP_CSV_NO_ESCAPE};
  if (!escape.empty()) {
    escape_char = static_cast<int32_t>(escape[0]);
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
    length = -2; // this is necessary to pass a negative number to fgets
  }
  Optional<string> line_optional{f$fgets(stream, length + 1)};
  if (!line_optional.has_value()) {
    return false;
  }

  mbstate_t ps{};
  return getcsv(stream, line_optional.val(), delimiter_char, enclosure_char, escape_char, std::addressof(ps));
}
