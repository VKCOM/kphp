// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt
#include "runtime-common/stdlib/vkext/vkext-functions.h"

#include <array>
#include <cstdio>

namespace {

constexpr std::array<int, 256> utf8_to_win_convert_0x400 = {
    -1,   0xa8, 0x80, 0x81, 0xaa, 0xbd, 0xb2, 0xaf, 0xa3, 0x8a, 0x8c, 0x8e, 0x8d, -1,   0xa1, 0x8f, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9,
    0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3,
    0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd,
    0xfe, 0xff, -1,   0xb8, 0x90, 0x83, 0xba, 0xbe, 0xb3, 0xbf, 0xbc, 0x9a, 0x9c, 0x9e, 0x9d, -1,   0xa2, 0x9f, -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   0xa5, 0xb4, -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1};
constexpr std::array<int, 256> utf8_to_win_convert_0x2000 = {
    -1,   -1,   -1, -1,   -1,   -1,   -1, -1, -1, -1,   -1, -1, -1, -1, -1,   -1,   -1, -1,   -1, 0x96, 0x97, -1, -1, -1, 0x91, 0x92, 0x82, -1,   0x93,
    0x94, 0x84, -1, 0x86, 0x87, 0x95, -1, -1, -1, 0x85, -1, -1, -1, -1, 0xda, 0xda, -1, 0xda, -1, 0x89, -1,   -1, -1, -1, -1,   -1,   -1,   -1,   0x8b,
    0x9b, -1,   -1, -1,   -1,   -1,   -1, -1, -1, -1,   -1, -1, -1, -1, -1,   -1,   -1, -1,   -1, -1,   -1,   -1, -1, -1, -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1, -1,   -1,   -1,   -1, -1, -1, -1,   -1, -1, -1, -1, -1,   -1,   -1, -1,   -1, -1,   -1,   -1, -1, -1, -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1, -1,   -1,   -1,   -1, -1, -1, -1,   -1, -1, -1, -1, -1,   -1,   -1, -1,   -1, -1,   -1,   -1, -1, -1, -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1, -1,   -1,   -1,   -1, -1, -1, -1,   -1, -1, -1, -1, -1,   -1,   -1, -1,   -1, -1,   -1,   -1, -1, -1, -1,   -1,   -1,   0x88, -1,
    -1,   -1,   -1, -1,   -1,   -1,   -1, -1, -1, -1,   -1, -1, -1, -1, -1,   -1,   -1, -1,   -1, -1,   -1,   -1, -1, -1, -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1, -1,   -1,   -1,   -1, -1, -1, -1,   -1, -1, -1, -1, -1,   -1,   -1, -1,   -1, -1,   -1,   -1, -1, -1, -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1, -1,   -1,   -1,   -1, -1, -1, -1,   -1, -1, -1, -1, -1,   -1,   -1, -1,   -1, -1,   -1,   -1, -1, -1};
constexpr std::array<int, 256> utf8_to_win_convert_0xff00 = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0x20, -1, -1};
constexpr std::array<int, 256> utf8_to_win_convert_0x2100 = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0xb9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0x99, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
constexpr std::array<int, 256> utf8_to_win_convert_0x000 = {
    -1,   -1, -1,   -1,   -1, -1, -1, -1,   -1,   -1,   -1, -1, -1, -1,   -1, -1,   -1, -1, -1, -1,   -1, -1,   -1,   -1, -1,   -1, -1,   -1,   -1,
    -1,   -1, -1,   -1,   -1, -1, -1, -1,   -1,   -1,   -1, -1, -1, -1,   -1, -1,   -1, -1, -1, -1,   -1, -1,   -1,   -1, -1,   -1, -1,   -1,   -1,
    -1,   -1, -1,   -1,   -1, -1, -1, -1,   -1,   -1,   -1, -1, -1, -1,   -1, -1,   -1, -1, -1, -1,   -1, -1,   -1,   -1, -1,   -1, -1,   -1,   -1,
    -1,   -1, -1,   -1,   -1, -1, -1, -1,   -1,   -1,   -1, -1, -1, -1,   -1, -1,   -1, -1, -1, -1,   -1, -1,   -1,   -1, -1,   -1, -1,   -1,   -1,
    -1,   -1, -1,   -1,   -1, -1, -1, -1,   -1,   -1,   -1, -1, -1, -1,   -1, -1,   -1, -1, -1, -1,   -1, -1,   -1,   -1, -1,   -1, -1,   -1,   -1,
    -1,   -1, -1,   -1,   -1, -1, -1, -1,   -1,   -1,   -1, -1, -1, -1,   -1, 0xa0, -1, -1, -1, 0xa4, -1, 0xa6, 0xa7, -1, 0xa9, -1, 0xab, 0xac, 0xad,
    0xae, -1, 0xb0, 0xb1, -1, -1, -1, 0xb5, 0xb6, 0xb7, -1, -1, -1, 0xbb, -1, -1,   -1, -1, -1, -1,   -1, -1,   -1,   -1, -1,   -1, -1,   -1,   -1,
    -1,   -1, -1,   -1,   -1, -1, -1, -1,   -1,   -1,   -1, -1, -1, -1,   -1, -1,   -1, -1, -1, -1,   -1, -1,   -1,   -1, -1,   -1, -1,   -1,   -1,
    -1,   -1, -1,   -1,   -1, -1, -1, -1,   -1,   -1,   -1, -1, -1, -1,   -1, -1,   -1, -1, -1, -1,   -1, -1,   -1,   -1};
constexpr std::array<int, 256> win_to_utf8_convert = {
    0x0,   0x1,   0x2,   0x3,   0x4,   0x5,    0x6,    0x7,    0x8,    0x9,    0xa,    0xb,    0xc,    0xd,    0xe,    0xf,    0x10,   0x11,   0x12,  0x13,
    0x14,  0x15,  0x16,  0x17,  0x18,  0x19,   0x1a,   0x1b,   0x1c,   0x1d,   0x1e,   0x1f,   0x20,   0x21,   0x22,   0x23,   0x24,   0x25,   0x26,  0x27,
    0x28,  0x29,  0x2a,  0x2b,  0x2c,  0x2d,   0x2e,   0x2f,   0x30,   0x31,   0x32,   0x33,   0x34,   0x35,   0x36,   0x37,   0x38,   0x39,   0x3a,  0x3b,
    0x3c,  0x3d,  0x3e,  0x3f,  0x40,  0x41,   0x42,   0x43,   0x44,   0x45,   0x46,   0x47,   0x48,   0x49,   0x4a,   0x4b,   0x4c,   0x4d,   0x4e,  0x4f,
    0x50,  0x51,  0x52,  0x53,  0x54,  0x55,   0x56,   0x57,   0x58,   0x59,   0x5a,   0x5b,   0x5c,   0x5d,   0x5e,   0x5f,   0x60,   0x61,   0x62,  0x63,
    0x64,  0x65,  0x66,  0x67,  0x68,  0x69,   0x6a,   0x6b,   0x6c,   0x6d,   0x6e,   0x6f,   0x70,   0x71,   0x72,   0x73,   0x74,   0x75,   0x76,  0x77,
    0x78,  0x79,  0x7a,  0x7b,  0x7c,  0x7d,   0x7e,   0x7f,   0x402,  0x403,  0x201a, 0x453,  0x201e, 0x2026, 0x2020, 0x2021, 0x20ac, 0x2030, 0x409, 0x2039,
    0x40a, 0x40c, 0x40b, 0x40f, 0x452, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x0,    0x2122, 0x459,  0x203a, 0x45a,  0x45c,  0x45b, 0x45f,
    0xa0,  0x40e, 0x45e, 0x408, 0xa4,  0x490,  0xa6,   0xa7,   0x401,  0xa9,   0x404,  0xab,   0xac,   0xad,   0xae,   0x407,  0xb0,   0xb1,   0x406, 0x456,
    0x491, 0xb5,  0xb6,  0xb7,  0x451, 0x2116, 0x454,  0xbb,   0x458,  0x405,  0x455,  0x457,  0x410,  0x411,  0x412,  0x413,  0x414,  0x415,  0x416, 0x417,
    0x418, 0x419, 0x41a, 0x41b, 0x41c, 0x41d,  0x41e,  0x41f,  0x420,  0x421,  0x422,  0x423,  0x424,  0x425,  0x426,  0x427,  0x428,  0x429,  0x42a, 0x42b,
    0x42c, 0x42d, 0x42e, 0x42f, 0x430, 0x431,  0x432,  0x433,  0x434,  0x435,  0x436,  0x437,  0x438,  0x439,  0x43a,  0x43b,  0x43c,  0x43d,  0x43e, 0x43f,
    0x440, 0x441, 0x442, 0x443, 0x444, 0x445,  0x446,  0x447,  0x448,  0x449,  0x44a,  0x44b,  0x44c,  0x44d,  0x44e,  0x44f};

int utf8_to_win_char(int c) noexcept {
  if (c < 0x80) {
    return c;
  }
  switch (c & ~0xff) {
  case 0x0400:
    return utf8_to_win_convert_0x400[c & 0xff];
  case 0x2000:
    return utf8_to_win_convert_0x2000[c & 0xff];
  case 0xff00:
    return utf8_to_win_convert_0xff00[c & 0xff];
  case 0x2100:
    return utf8_to_win_convert_0x2100[c & 0xff];
  case 0x0000:
    return utf8_to_win_convert_0x000[c & 0xff];
  }
  return -1;
}

struct buffered_write {
  string result;

  void write_buff(const char* s, size_t l) noexcept {
    result.append(s, l);
  }

  void write_buff_set_pos(size_t pos) noexcept {
    result.shrink(pos);
  }

  void write_buff_char_pos(char c, size_t pos) noexcept {
    if (pos < result.size()) {
      result[pos] = c;
      return;
    }
  }

  void write_buff_char(char c) noexcept {
    result.push_back(c);
  }

  void write_buff_char_2(char c1, char c2) noexcept {
    result.push_back(c1);
    result.push_back(c2);
  }

  void write_buff_char_3(char c1, char c2, char c3) noexcept {
    result.push_back(c1);
    result.push_back(c2);
    result.push_back(c3);
  }

  void write_buff_char_4(char c1, char c2, char c3, char c4) noexcept {
    result.push_back(c1);
    result.push_back(c2);
    result.push_back(c3);
    result.push_back(c4);
  }

  void write_buff_char_5(char c1, char c2, char c3, char c4, char c5) noexcept {
    result.push_back(c1);
    result.push_back(c2);
    result.push_back(c3);
    result.push_back(c4);
    result.push_back(c5);
  }

  void write_buff_char_6(char c1, char c2, char c3, char c4, char c5, char c6) noexcept {
    result.push_back(c1);
    result.push_back(c2);
    result.push_back(c3);
    result.push_back(c4);
    result.push_back(c5);
    result.push_back(c6);
  }

  void write_buff_int(int x) noexcept {
    char buff[25];
    auto len = snprintf(buff, 25, "%d", x);
    result.append(buff, len);
  }

  int utf8_to_win(const char* s, int len, int64_t max_len, bool exit_on_error) noexcept {
    int st = 0;
    int acc = 0;
    //  if (max_len && len > 3 * max_len) {
    //    len = 3 * max_len;
    //  }
    for (int i = 0; i < len; i++) {
      if (max_len && result.size() >= max_len) {
        break;
      }
      int c = static_cast<unsigned char>(s[i]);
      if (c < 0x80) {
        if (st) {
          if (exit_on_error) {
            return -1;
          }
          write_buff("?1?", 3);
        }
        write_buff_char(static_cast<char>(c));
        st = 0;
      } else if ((c & 0xc0) == 0x80) {
        if (!st) {
          if (exit_on_error) {
            return -1;
          }
          write_buff("?2?", 3);
          continue;
        }
        acc <<= 6;
        acc += c - 0x80;
        if (!--st) {
          if (acc < 0x80) {
            if (exit_on_error) {
              return -1;
            }
            write_buff("?3?", 3);
          } else {
            int d = utf8_to_win_char(acc);
            if (d != -1 && d) {
              write_buff_char(static_cast<char>(d));
            } else {
              write_buff_char_2('&', '#');
              write_buff_int(acc);
              write_buff_char(';');
            }
          }
        }
      } else { // if ((c & 0xc0) == 0xc0)
        if (st) {
          if (exit_on_error) {
            return -1;
          }
          write_buff("?4?", 3);
        }
        c -= 0xc0;
        st = 0;
        if (c < 32) {
          acc = c;
          st = 1;
        } else if (c < 48) {
          acc = c - 32;
          st = 2;
        } else if (c < 56) {
          acc = c - 48;
          st = 3;
        } else {
          if (exit_on_error) {
            return -1;
          }
          write_buff("?5?", 3);
        }
      }
    }
    if (st) {
      if (exit_on_error) {
        return -1;
      }
      write_buff("?6?", 3);
    }
    return 1;
  }

  void write_char_utf8(int64_t c) noexcept {
    if (!c) {
      return;
    }
    if (c < 128) {
      write_buff_char(static_cast<char>(c));
      return;
    }
    // 2 bytes(11): 110x xxxx 10xx xxxx
    if (c < 0x800) {
      write_buff_char_2(static_cast<char>(0xC0 + (c >> 6)), static_cast<char>(0x80 + (c & 63)));
      return;
    }

    // 3 bytes(16): 1110 xxxx 10xx xxxx 10xx xxxx
    if (c < 0x10000) {
      write_buff_char_3(static_cast<char>(0xE0 + (c >> 12)), static_cast<char>(0x80 + ((c >> 6) & 63)), static_cast<char>(0x80 + (c & 63)));
      return;
    }

    // 4 bytes(21): 1111 0xxx 10xx xxxx 10xx xxxx 10xx xxxx
    if (c < 0x200000) {
      write_buff_char_4(static_cast<char>(0xF0 + (c >> 18)), static_cast<char>(0x80 + ((c >> 12) & 63)), static_cast<char>(0x80 + ((c >> 6) & 63)),
                        static_cast<char>(0x80 + (c & 63)));
      return;
    }

    // 5 bytes(26): 1111 10xx 10xx xxxx 10xx xxxx 10xx xxxx 10xx xxxx
    if (c < 0x4000000) {
      write_buff_char_5(static_cast<char>(0xF8 + (c >> 24)), static_cast<char>(0x80 + ((c >> 18) & 63)), static_cast<char>(0x80 + ((c >> 12) & 63)),
                        static_cast<char>(0x80 + ((c >> 6) & 63)), static_cast<char>(0x80 + (c & 63)));
      return;
    }

    // 6 bytes(31): 1111 110x 10xx xxxx 10xx xxxx 10xx xxxx 10xx xxxx 10xx xxxx
    if (c < 0x80000000) {
      write_buff_char_6(static_cast<char>(0xFC + (c >> 30)), static_cast<char>(0x80 + ((c >> 24) & 63)), static_cast<char>(0x80 + ((c >> 18) & 63)),
                        static_cast<char>(0x80 + ((c >> 12) & 63)), static_cast<char>(0x80 + ((c >> 6) & 63)), static_cast<char>(0x80 + (c & 63)));
      return;
    }

    write_buff_char_2('$', '#');
    write_buff_int(c);
    write_buff_char(';');
  }

  void write_char_utf8_no_escape(int64_t c) noexcept {
    if (!c) {
      return;
    }
    if (c < 128) {
      write_buff_char(static_cast<char>(c));
      return;
    }
    // 2 bytes(11): 110x xxxx 10xx xxxx
    if (c < 0x800) {
      write_buff_char_2(static_cast<char>(0xC0 + (c >> 6)), static_cast<char>(0x80 + (c & 63)));
      return;
    }

    // 3 bytes(16): 1110 xxxx 10xx xxxx 10xx xxxx
    if (c < 0x10000) {
      write_buff_char_3(static_cast<char>(0xE0 + (c >> 12)), static_cast<char>(0x80 + ((c >> 6) & 63)), static_cast<char>(0x80 + (c & 63)));
      return;
    }

    // 4 bytes(21): 1111 0xxx 10xx xxxx 10xx xxxx 10xx xxxx
    if (c < 0x200000) {
      write_buff_char_4(static_cast<char>(0xF0 + (c >> 18)), static_cast<char>(0x80 + ((c >> 12) & 63)), static_cast<char>(0x80 + ((c >> 6) & 63)),
                        static_cast<char>(0x80 + (c & 63)));
      return;
    }

    // 5 bytes(26): 1111 10xx 10xx xxxx 10xx xxxx 10xx xxxx 10xx xxxx
    if (c < 0x4000000) {
      write_buff_char_5(static_cast<char>(0xF8 + (c >> 24)), static_cast<char>(0x80 + ((c >> 18) & 63)), static_cast<char>(0x80 + ((c >> 12) & 63)),
                        static_cast<char>(0x80 + ((c >> 6) & 63)), static_cast<char>(0x80 + (c & 63)));
      return;
    }

    // 6 bytes(31): 1111 110x 10xx xxxx 10xx xxxx 10xx xxxx 10xx xxxx 10xx xxxx
    if (c < 0x80000000) {
      write_buff_char_6(static_cast<char>(0xFC + (c >> 30)), static_cast<char>(0x80 + ((c >> 24) & 63)), static_cast<char>(0x80 + ((c >> 18) & 63)),
                        static_cast<char>(0x80 + ((c >> 12) & 63)), static_cast<char>(0x80 + ((c >> 6) & 63)), static_cast<char>(0x80 + (c & 63)));
      return;
    }
  }

  void win_to_utf8(const char* s, int len, bool escape) noexcept {
    int state = 0;
    int save_pos = -1;
    int64_t cur_num = 0;
    for (int i = 0; i < len; i++) {
      if (state == 0 && s[i] == '&') {
        save_pos = result.size();
        cur_num = 0;
        state++;
      } else if (state == 1 && s[i] == '#') {
        state++;
      } else if (state == 2 && s[i] >= '0' && s[i] <= '9') {
        if (cur_num < 0x80000000) {
          cur_num = s[i] - '0' + cur_num * 10;
        }
      } else if (state == 2 && s[i] == ';') {
        state++;
      } else {
        state = 0;
      }
      if (state == 3 && 0xd800 <= cur_num && cur_num <= 0xdfff) {
        cur_num = 32;
      }
      if (state == 3 && (!escape || (cur_num >= 32 && cur_num != 33 && cur_num != 34 && cur_num != 36 && cur_num != 39 && cur_num != 60 && cur_num != 62 &&
                                     cur_num != 92 && cur_num != 8232 && cur_num != 8233 && cur_num < 0x80000000))) {
        write_buff_set_pos(save_pos);
        php_assert(save_pos == result.size());
        if (escape) {
          write_char_utf8(cur_num);
        } else {
          write_char_utf8_no_escape(cur_num);
        };
      } else if (state == 3 && cur_num >= 0x80000000) {
        write_char_utf8(win_to_utf8_convert[static_cast<unsigned char>(s[i])]);
        write_buff_char_pos('$', save_pos);
      } else {
        write_char_utf8(win_to_utf8_convert[static_cast<unsigned char>(s[i])]);
      }
      if (state == 3) {
        state = 0;
      }
    }
  }
};

constexpr std::array<char, 256> ws = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};

bool is_html_opt_symb(char c) noexcept {
  return (c == '<' || c == '>' || c == '(' || c == ')' || c == '{' || c == '}' || c == '/' || c == '"' || c == ':' || c == ',' || c == ';');
}

bool is_space(char c) noexcept {
  return ws[static_cast<unsigned char>(c)];
}

bool is_linebreak(char c) noexcept {
  return c == '\n';
}

bool is_pre_tag(const char* s) noexcept {
  if (s[0] == '<') {
    if (s[1] == 'p') {
      return s[2] == 'r' && s[3] == 'e' && s[4] == '>';
    } else if (s[1] == 'c') {
      return s[2] == 'o' && s[3] == 'd' && s[4] == 'e' && s[5] == '>';
    } else if (s[1] == '/') {
      if (s[2] == 'p') {
        return -(s[3] == 'r' && s[4] == 'e' && s[5] == '>');
      } else if (s[2] == 'c') {
        return -(s[3] == 'o' && s[4] == 'd' && s[5] == 'e' && s[6] == '>');
      }
    }
  }
  return false;
}
} // namespace

string f$vk_utf8_to_win(const string& text, int64_t max_len, bool exit_on_error) noexcept {
  buffered_write buffered{};
  int r = buffered.utf8_to_win(text.c_str(), text.size(), max_len, exit_on_error);
  if (r >= 0) {
    return buffered.result;
  } else {
    if (!max_len || text.size() <= static_cast<string::size_type>(max_len)) {
      return text;
    }
    return {text.c_str(), static_cast<string::size_type>(max_len)};
  }
}

string f$vk_win_to_utf8(const string& text, bool escape) noexcept {
  buffered_write buffered{};
  buffered.win_to_utf8(text.c_str(), text.size(), escape);
  return buffered.result;
}

string f$vk_whitespace_pack(const string& str, bool html_opt) noexcept {
  const char* text = str.c_str();
  int level = 0;
  const char* ctext = text;
  const char* start = text;

  buffered_write buffered{};
  while (*text) {
    if (is_space(*text) && !level) {
      int linebreak = 0;
      while (is_space(*text)) {
        if (is_linebreak(*text)) {
          linebreak = 1;
        }
        text++;
      }
      if (!html_opt || (ctext != start && !is_html_opt_symb(ctext[-1]) && *text && !is_html_opt_symb(*text))) {
        buffered.write_buff_char(linebreak ? '\n' : ' ');
      }
    } else {
      while (true) {
        while ((level || !is_space(*text)) && *text) {
          level += is_pre_tag(text);
          if (level < 0) {
            level = 1000000000;
          }
          text++;
        }
        if (!html_opt && *text && !is_space(text[1])) {
          text++;
        } else {
          break;
        }
      }
      buffered.write_buff(ctext, text - ctext);
    }
    ctext = text;
  }
  return buffered.result;
}
