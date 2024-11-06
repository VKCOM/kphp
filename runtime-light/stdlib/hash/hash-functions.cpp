// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/hash/hash-functions.h"

static int base64_encode(const unsigned char *const input, int ilen, char *output, int olen) {
  static const char *const symbols64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

  auto next_input_uchar = [input, ilen](int &i) {
    if (i >= ilen) {
      return static_cast<unsigned char>(0);
    }
    return input[i++];
  };

  int j = 0;
  char buf[4];
  for (int i = 0; i < ilen;) {
    int old_i = i;
    int o = next_input_uchar(i);
    o <<= 8;
    o |= next_input_uchar(i);
    o <<= 8;
    o |= next_input_uchar(i);
    int l = i - old_i;
    assert(l > 0 && l <= 3);
    for (int u = 3; u >= 0; u--) {
      buf[u] = symbols64[o & 63];
      o >>= 6;
    }
    if (l == 1) {
      buf[2] = buf[3] = '=';
    } else if (l == 2) {
      buf[3] = '=';
    }
    if (j + 3 >= olen) {
      return -1;
    }
    memcpy(&output[j], buf, 4);
    j += 4;
  }
  if (j >= olen) {
    return -1;
  }
  output[j++] = 0;
  return 0;
}

string f$base64_encode(const string &s) noexcept {
  int result_len = (s.size() + 2) / 3 * 4;
  string res(result_len, false);
  result_len = base64_encode(reinterpret_cast<const unsigned char *>(s.c_str()), static_cast<int>(s.size()), res.buffer(), result_len + 1);

  if (result_len != 0) {
    return {};
  }

  return res;
}

static constexpr short base64_reverse_table[256] = {-2, -2, -2, -2, -2, -2, -2, -2, -2, -1, -1, -2, -2, -1, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
                                                    -2, -2, -2, -2, -2, -2, -1, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, 62, -2, -2, -2, 63, 52, 53, 54, 55,
                                                    56, 57, 58, 59, 60, 61, -2, -2, -2, -2, -2, -2, -2, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
                                                    13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -2, -2, -2, -2, -2, -2, 26, 27, 28, 29, 30, 31, 32,
                                                    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -2, -2, -2, -2, -2, -2, -2,
                                                    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
                                                    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
                                                    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
                                                    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
                                                    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2};

/* Function f$base64_decode ported from https://github.com/php/php-src/blob/master/ext/standard/base64.c#L130
 * "This product includes PHP software, freely available from <http://www.php.net/software/>".
 */
Optional<string> f$base64_decode(const string &s, bool strict) noexcept {
  /* run through the whole string, converting as we go */
  string::size_type result_len = (s.size() + 3) / 4 * 3;
  string result(result_len, false);
  int i = 0;
  string::size_type j = 0;
  int padding = 0;
  for (string::size_type pos = 0; pos < s.size(); pos++) {
    int ch = static_cast<unsigned char>(s[pos]);
    if (ch == '=') {
      padding++;
      continue;
    }

    ch = base64_reverse_table[ch];
    if (!strict) {
      /* skip unknown characters and whitespace */
      if (ch < 0) {
        continue;
      }
    } else {
      /* skip whitespace */
      if (ch == -1) {
        continue;
      }
      /* fail on bad characters or if any data follows padding */
      if (ch == -2 || padding) {
        return false;
      }
    }

    switch (i % 4) {
      case 0:
        result[j] = static_cast<unsigned char>(ch << 2);
        break;
      case 1:
        result[j] = static_cast<char>(result[j] | (ch >> 4));
        result[++j] = static_cast<unsigned char>((ch & 0x0f) << 4);
        break;
      case 2:
        result[j] = static_cast<char>(result[j] | (ch >> 2));
        result[++j] = static_cast<unsigned char>((ch & 0x03) << 6);
        break;
      case 3:
        result[j] = static_cast<char>(result[j] | ch);
        ++j;
        break;
    }
    i++;
  }
  /* fail if the input is truncated (only one char in last group) */
  if (strict && i % 4 == 1) {
    return false;
  }
  /* fail if the padding length is wrong (not VV==, VVV=), but accept zero padding
   * RFC 4648: "In some circumstances, the use of padding [--] is not required" */
  if (strict && padding && (padding > 2 || (i + padding) % 4 != 0)) {
    return false;
  }

  result.shrink(j);

  return result;
}
