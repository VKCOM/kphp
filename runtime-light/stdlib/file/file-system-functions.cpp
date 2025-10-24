//
// Created by nsinyachenko on 24.10.25.
//

#include "runtime-light/stdlib/file/file-system-functions.h"

static constexpr int64_t IMAGETYPE_UNKNOWN = 0;
static constexpr int64_t IMAGETYPE_GIF = 1;
static constexpr int64_t IMAGETYPE_JPEG = 2;
static constexpr int64_t IMAGETYPE_PNG = 3;
static constexpr int64_t IMAGETYPE_SWF = 4;
static constexpr int64_t IMAGETYPE_PSD = 5;
static constexpr int64_t IMAGETYPE_BMP = 6;
static constexpr int64_t IMAGETYPE_TIFF_II = 7;
static constexpr int64_t IMAGETYPE_TIFF_MM = 8;
static constexpr int64_t IMAGETYPE_JPC = 9;
static constexpr int64_t IMAGETYPE_JPEG2000 = 9;
static constexpr int64_t IMAGETYPE_JP2 = 10;

static constexpr std::array<char, 3> php_sig_gif = {'G', 'I', 'F'};
static constexpr std::array<char, 3> php_sig_jpg = {(char)0xff, (char)0xd8, (char)0xff};
static constexpr std::array<char, 8> php_sig_png = {(char)0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
static constexpr std::array<char, 4> php_sig_jpc = {(char)0xff, 0x4f, (char)0xff, 0x51};
static constexpr std::array<char, 12> php_sig_jp2 = {0x00, 0x00, 0x00, 0x0c, 'j', 'P', ' ', ' ', 0x0d, 0x0a, (char)0x87, 0x0a};

static constexpr std::array<std::string_view, 11> mime_type_string = {
    "",          "image/gif",      "image/jpeg", "image/png",  "application/x-shockwave-flash",
    "image/psd", "image/x-ms-bmp", "image/tiff", "image/tiff", "application/octet-stream",
    "image/jp2"};

static constexpr int M_SOF0 = 0xC0;
static constexpr int M_SOF1 = 0xC1;
static constexpr int M_SOF2 = 0xC2;
static constexpr int M_SOF3 = 0xC3;
static constexpr int M_SOF5 = 0xC5;
static constexpr int M_SOF6 = 0xC6;
static constexpr int M_SOF7 = 0xC7;
static constexpr int M_SOF9 = 0xC9;
static constexpr int M_SOF10 = 0xCA;
static constexpr int M_SOF11 = 0xCB;
static constexpr int M_SOF13 = 0xCD;
static constexpr int M_SOF14 = 0xCE;
static constexpr int M_SOF15 = 0xCF;
static constexpr int M_EOI = 0xD9;
static constexpr int M_SOS = 0xDA;
static constexpr int M_COM = 0xFE;

static constexpr int M_PSEUDO = 0xFFD8;

inline mixed f$getimagesize(const string& name) {
  // TODO implement k2_fstat, with fd as parameter !!!
  struct stat stat_buf;
  if (k2_stat(name.c_str(), name.size(), &stat_buf) != k2::errno_ok) {
    return false;
  }

  auto sync_res{from_mixed<class_instance<kphp::fs::sync_resource>>(f$fopen(name, READ_MODE), {})};
  if (sync_res.is_null()) {
    return false;
  }
  auto f = sync_res.get();

  if (!S_ISREG(stat_buf.st_mode)) {
    kphp::log::warning("Regular file expected as first argument in function getimagesize, \"%s\" is given", name.c_str());
    return false;
  }

  constexpr size_t min_size = 3 * 256 + 64;
  std::array<unsigned char, min_size> buf;
  size_t size = stat_buf.st_size;
  size_t read_size = min_size;
  if (size < min_size) {
    read_size = size;
  }
  std::span<unsigned char> buf_span(buf.begin(), min_size);

  if (read_size < 12) {
    return false;
  }
  std::expected<size_t, int32_t> read_res = read_safe(f, std::as_writable_bytes(buf_span));
  if (!read_res || *read_res < read_size) {
    return false;
  }

  int width = 0;
  int height = 0;
  int bits = 0;
  int channels = 0;
  int type = IMAGETYPE_UNKNOWN;
  switch (buf[0]) {
  case 'G': // gif
    if (!std::strncmp((const char*)buf.begin(), php_sig_gif.begin(), sizeof(php_sig_gif))) {
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
    if (!std::strncmp((const char*)buf.begin(), php_sig_jpg.begin(), sizeof(php_sig_jpg))) {
      type = IMAGETYPE_JPEG;

      unsigned char* image = (unsigned char*)RuntimeAllocator::get().alloc_script_memory(size);
      if (image == nullptr) {
        kphp::log::warning("Not enough memory to process file \"%s\" in getimagesize", name.c_str());
        return false;
      }
      memcpy(image, buf.begin(), read_size);

      std::span<unsigned char> image_span(image + read_size, size - read_size);
      read_res = read_safe(f, std::as_writable_bytes(image_span));
      if (!read_res || *read_res < size - read_size) {
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
    } else if (!std::strncmp((const char*)buf.begin(), php_sig_jpc.begin(), sizeof(php_sig_jpc)) && static_cast<int>(read_size) >= 42) {
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
    if (read_size >= 54 && !std::strncmp((const char*)buf.begin(), php_sig_jp2.begin(), sizeof(php_sig_jp2))) {
      type = IMAGETYPE_JP2;

      bool found = false;

      int buf_pos = 12;
      size_t file_pos = 12;
      while (static_cast<int>(read_size) >= 42 + buf_pos + 8) {
        const unsigned char* s = buf.begin() + buf_pos;
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
        std::span<unsigned char> buf_read_span(buf.begin(), read_size);
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
    if (read_size >= 25 && !std::strncmp((const char*)buf.begin(), php_sig_png.begin(), sizeof(php_sig_png))) {
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

  string::size_type len = std::format_to_n(reinterpret_cast<char*>(buf.data()), min_size, "width=\"%d\" height=\"%d\"", width, height).size;
  result.push_back(string{(const char*)buf.begin(), len});
  if (bits != 0) {
    result.set_value(string{"bits", 4}, bits);
  }
  if (channels != 0) {
    result.set_value(string{"channels", 8}, channels);
  }
  result.set_value(string{"mime", 4}, string{mime_type_string[type].data(), static_cast<string::size_type>(mime_type_string[type].size())});

  return result;
}
