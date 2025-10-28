// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <array>
#include <expected>
#include <span>
#include <string_view>
#include <sys/stat.h>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/file/file-system-functions.h"

namespace {
constexpr int64_t IMAGETYPE_UNKNOWN = 0;
constexpr int64_t IMAGETYPE_GIF = 1;
constexpr int64_t IMAGETYPE_JPEG = 2;
constexpr int64_t IMAGETYPE_PNG = 3;
// constexpr int64_t IMAGETYPE_SWF = 4;
// constexpr int64_t IMAGETYPE_PSD = 5;
// constexpr int64_t IMAGETYPE_BMP = 6;
// constexpr int64_t IMAGETYPE_TIFF_II = 7;
// constexpr int64_t IMAGETYPE_TIFF_MM = 8;
// constexpr int64_t IMAGETYPE_JPC = 9;
// constexpr int64_t IMAGETYPE_JPEG2000 = 9;
constexpr int64_t IMAGETYPE_JP2 = 10;

constexpr std::array<char, 3> php_sig_gif = {'G', 'I', 'F'};
constexpr std::array<char, 3> php_sig_jpg = {static_cast<char>(0xff), static_cast<char>(0xd8), static_cast<char>(0xff)};
constexpr std::array<char, 8> php_sig_png = {static_cast<char>(0x89), 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
constexpr std::array<char, 4> php_sig_jpc = {static_cast<char>(0xff), 0x4f, static_cast<char>(0xff), 0x51};
constexpr std::array<char, 12> php_sig_jp2 = {0x00, 0x00, 0x00, 0x0c, 'j', 'P', ' ', ' ', 0x0d, 0x0a, static_cast<char>(0x87), 0x0a};

constexpr std::array<std::string_view, 11> mime_type_string = {"",          "image/gif",      "image/jpeg", "image/png",  "application/x-shockwave-flash",
                                                               "image/psd", "image/x-ms-bmp", "image/tiff", "image/tiff", "application/octet-stream",
                                                               "image/jp2"};

constexpr int32_t M_SOF0 = 0xC0;
constexpr int32_t M_SOF1 = 0xC1;
constexpr int32_t M_SOF2 = 0xC2;
constexpr int32_t M_SOF3 = 0xC3;
constexpr int32_t M_SOF5 = 0xC5;
constexpr int32_t M_SOF6 = 0xC6;
constexpr int32_t M_SOF7 = 0xC7;
constexpr int32_t M_SOF9 = 0xC9;
constexpr int32_t M_SOF10 = 0xCA;
constexpr int32_t M_SOF11 = 0xCB;
constexpr int32_t M_SOF13 = 0xCD;
constexpr int32_t M_SOF14 = 0xCE;
constexpr int32_t M_SOF15 = 0xCF;
constexpr int32_t M_EOI = 0xD9;
constexpr int32_t M_SOS = 0xDA;
constexpr int32_t M_COM = 0xFE;

constexpr int32_t M_PSEUDO = 0xFFD8;
}; // namespace

mixed f$getimagesize(const string& name) noexcept {
  // TODO implement k2_fstat, with fd as parameter !!!
  struct stat stat_buf {};
  if (k2_stat(name.c_str(), name.size(), &stat_buf) != k2::errno_ok) {
    return false;
  }

  auto open_res{kphp::fs::file::open(std::string_view{name.c_str(), name.size()}, file_system_impl_::READ_MODE)};
  if (!open_res) {
    return false;
  }
  auto file{std::move(*open_res)};

  if (!S_ISREG(stat_buf.st_mode)) {
    kphp::log::warning("regular file expected as first argument in function getimagesize, \"{}\" is given", name.c_str());
    return false;
  }

  constexpr size_t min_size = 3 * 256 + 64;
  std::array<unsigned char, min_size> buf{};
  size_t size = stat_buf.st_size;
  size_t read_size = min_size;
  if (size < min_size) {
    read_size = size;
  }
  std::span<unsigned char> buf_span(buf.begin(), min_size);

  if (read_size < 12) {
    return false;
  }
  std::expected<size_t, int32_t> read_res = file_system_impl_::read_safe(file, std::as_writable_bytes(buf_span));
  if (!read_res || *read_res < read_size) {
    return false;
  }

  int32_t width = 0;
  int32_t height = 0;
  int32_t bits = 0;
  int32_t channels = 0;
  int32_t type = IMAGETYPE_UNKNOWN;
  switch (buf[0]) {
  case 'G': // gif
    if (!std::strncmp(reinterpret_cast<const char*>(buf.begin()), php_sig_gif.begin(), sizeof(php_sig_gif))) {
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
    if (!std::strncmp(reinterpret_cast<const char*>(buf.begin()), php_sig_jpg.begin(), sizeof(php_sig_jpg))) {
      type = IMAGETYPE_JPEG;

      auto* image = static_cast<unsigned char*>(RuntimeAllocator::get().alloc_script_memory(size));
      if (image == nullptr) {
        kphp::log::warning("not enough memory to process file \"{}\" in getimagesize", name.c_str());
        return false;
      }

      auto image_deleter = [size](void* ptr) { RuntimeAllocator::get().free_script_memory(ptr, size); };
      std::unique_ptr<void, decltype(image_deleter)> image_unique_ptr{image, std::move(image_deleter)};

      std::memcpy(image, buf.begin(), read_size);

      std::span<unsigned char> image_span(image + read_size, size - read_size);
      read_res = file_system_impl_::read_safe(file, std::as_writable_bytes(image_span));
      if (!read_res || *read_res < size - read_size) {
        return false;
      }

      int32_t marker = M_PSEUDO;
      size_t cur_pos = 2;

      while (height == 0 && width == 0 && marker != M_SOS && marker != M_EOI) {
        int32_t a = 0;
        int32_t comment_correction = 1 + (marker == M_COM);
        int32_t new_marker = 0;

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
            return false;
          }
          cur_pos += length;
          break;
        }
        }
      }
    } else if (!std::strncmp(reinterpret_cast<const char*>(buf.begin()), php_sig_jpc.begin(), sizeof(php_sig_jpc)) && static_cast<int>(read_size) >= 42) {
      type = IMAGETYPE_JPEG;

      width = (buf[8] << 24) + (buf[9] << 16) + (buf[10] << 8) + buf[11];
      height = (buf[12] << 24) + (buf[13] << 16) + (buf[14] << 8) + buf[15];
      channels = (buf[40] << 8) + buf[41];

      if (channels < 0 || channels > 256 || static_cast<int>(read_size) < 42 + 3 * channels || width <= 0 || height <= 0) {
        return false;
      }

      bits = 0;
      for (int32_t i = 0; i < channels; i++) {
        int32_t cur_bits = buf[42 + 3 * i];
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
    if (read_size >= 54 && !std::strncmp(reinterpret_cast<const char*>(buf.begin()), php_sig_jp2.begin(), sizeof(php_sig_jp2))) {
      type = IMAGETYPE_JP2;

      bool found = false;

      int32_t buf_pos = 12;
      size_t file_pos = 12;
      while (static_cast<int>(read_size) >= 42 + buf_pos + 8) {
        const unsigned char* s = buf.begin() + buf_pos;
        int32_t box_length = (s[0] << 24) + (s[1] << 16) + (s[2] << 8) + s[3];
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
          for (int32_t i = 0; i < channels; i++) {
            int32_t cur_bits = s[42 + 3 * i];
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
        std::span<unsigned char> buf_read_span(buf.begin(), read_size);
        read_res = file.pread(std::as_writable_bytes(buf_read_span), static_cast<off_t>(file_pos));
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
    if (read_size >= 25 && !std::strncmp(reinterpret_cast<const char*>(buf.begin()), php_sig_png.begin(), sizeof(php_sig_png))) {
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

  array<mixed> result(array_size(7, false));
  result.push_back(width);
  result.push_back(height);
  result.push_back(type);

  string::size_type len = std::distance(reinterpret_cast<char*>(buf.data()),
                                        std::format_to_n(reinterpret_cast<char*>(buf.data()), min_size, R"(width="{}" height="{}")", width, height).out);
  result.push_back(string{reinterpret_cast<const char*>(buf.begin()), len});
  if (bits != 0) {
    result.set_value(string{"bits", 4}, bits);
  }
  if (channels != 0) {
    result.set_value(string{"channels", 8}, channels);
  }
  result.set_value(string{"mime", 4}, string{mime_type_string[type].data(), static_cast<string::size_type>(mime_type_string[type].size())});

  return result;
}
