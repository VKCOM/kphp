// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/server/url-functions.h"

#include <cinttypes>
#include <cstdint>
#include <cstring>
#include <string_view>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/string/string-context.h"
#include "runtime-common/stdlib/string/string-functions.h"

namespace {

constexpr short base64_reverse_table[256] = {-2, -2, -2, -2, -2, -2, -2, -2, -2, -1, -1, -2, -2, -1, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
                                             -2, -2, -2, -1, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, 62, -2, -2, -2, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
                                             -2, -2, -2, -2, -2, -2, -2, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                                             22, 23, 24, 25, -2, -2, -2, -2, -2, -2, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
                                             45, 46, 47, 48, 49, 50, 51, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
                                             -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
                                             -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
                                             -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
                                             -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2};

constexpr std::string_view good_url_symbols_mask = "00000000000000000000000000000000"
                                                   "00000000000001101111111111000000"
                                                   "01111111111111111111111111100001"
                                                   "01111111111111111111111111100000"
                                                   "00000000000000000000000000000000"
                                                   "00000000000000000000000000000000"
                                                   "00000000000000000000000000000000"
                                                   "00000000000000000000000000000000"; //[0-9a-zA-Z-_.]

int32_t base64_encode(const unsigned char *input, int32_t ilen, char *output, int32_t olen) noexcept {
  static constexpr std::string_view symbols64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

  auto next_input_uchar = [input, ilen](int32_t &i) {
    if (i >= ilen) {
      return static_cast<unsigned char>(0);
    }
    return input[i++];
  };

  int32_t j = 0;
  char buf[4];
  for (int32_t i = 0; i < ilen;) {
    int32_t old_i = i;
    int32_t o = next_input_uchar(i);
    o <<= 8;
    o |= next_input_uchar(i);
    o <<= 8;
    o |= next_input_uchar(i);
    int32_t l = i - old_i;
    assert(l > 0 && l <= 3);
    for (int32_t u = 3; u >= 0; u--) {
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
    std::memcpy(&output[j], buf, 4);
    j += 4;
  }
  if (j >= olen) {
    return -1;
  }
  output[j++] = 0;
  return 0;
}

// returns associative array like [host=> path=>] or empty array if str is not a valid url
array<mixed> php_url_parse_ex(const char *str, string::size_type length) noexcept {
  array<mixed> result;
  const char *s = str;
  const char *e = nullptr;
  const char *p = nullptr;
  const char *pp = nullptr;
  const char *ue = str + length;

  /* parse scheme */
  if ((e = static_cast<const char *>(memchr(s, ':', length))) && e != s) {
    /* validate scheme */
    p = s;
    while (p < e) {
      /* scheme = 1*[ lowalpha | digit | "+" | "-" | "." ] */
      if (!isalpha(*p) && !isdigit(*p) && *p != '+' && *p != '.' && *p != '-') {
        if (e + 1 < ue && e < s + strcspn(s, "?#")) {
          goto parse_port;
        } else if (s + 1 < ue && *s == '/' && *(s + 1) == '/') { /* relative-scheme URL */
          s += 2;
          e = nullptr;
          goto parse_host;
        } else {
          goto just_path;
        }
      }
      p++;
    }

    if (e + 1 == ue) { /* only scheme is available */
      result.set_value(string("scheme", 6), string(s, static_cast<string::size_type>(e - s)));
      return result;
    }

    /*
     * certain schemas like mailto: and zlib: may not have any / after them
     * this check ensures we support those.
     */
    if (*(e + 1) != '/') {
      /* check if the data we get is a port this allows us to
       * correctly parse things like a.com:80
       */
      p = e + 1;
      while (p < ue && isdigit(*p)) {
        p++;
      }

      if ((p == ue || *p == '/') && (p - e) < 7) {
        goto parse_port;
      }

      result.set_value(string("scheme", 6), string(s, static_cast<string::size_type>(e - s)));
      s = e + 1;
      goto just_path;
    } else {
      result.set_value(string("scheme", 6), string(s, static_cast<string::size_type>(e - s)));

      if (e + 2 < ue && *(e + 2) == '/') {
        s = e + 3;
        if (!strcmp(result.get_value(string("scheme", 6)).as_string().c_str(), "file")) {
          if (e + 3 < ue && *(e + 3) == '/') {
            /* support windows drive letters as in: file:///c:/somedir/file.txt */
            if (e + 5 < ue && *(e + 5) == ':') {
              s = e + 4;
            }
            goto just_path;
          }
        }
      } else {
        s = e + 1;
        goto just_path;
      }
    }
  } else if (e) { /* no scheme; starts with colon: look for port */
  parse_port:
    p = e + 1;
    pp = p;

    while (pp < ue && pp - p < 6 && isdigit(*pp)) {
      pp++;
    }

    if (pp - p > 0 && pp - p < 6 && (pp == ue || *pp == '/')) {
      int64_t port = string(p, static_cast<string::size_type>(pp - p)).to_int();
      if (port > 0 && port <= 65535) {
        result.set_value(string("port", 4), port);
        if (s + 1 < ue && *s == '/' && *(s + 1) == '/') { /* relative-scheme URL */
          s += 2;
        }
      } else {
        return {};
      }
    } else if (p == pp && pp == ue) {
      return {};
    } else if (s + 1 < ue && *s == '/' && *(s + 1) == '/') { /* relative-scheme URL */
      s += 2;
    } else {
      goto just_path;
    }
  } else if (s + 1 < ue && *s == '/' && *(s + 1) == '/') { /* relative-scheme URL */
    s += 2;
  } else {
    goto just_path;
  }

parse_host:
  /* Binary-safe strcspn(s, "/?#") */
  e = ue;
  if ((p = static_cast<const char *>(memchr(s, '/', e - s)))) {
    e = p;
  }
  if ((p = static_cast<const char *>(memchr(s, '?', e - s)))) {
    e = p;
  }
  if ((p = static_cast<const char *>(memchr(s, '#', e - s)))) {
    e = p;
  }

  /* check for login and password */
  if ((p = static_cast<const char *>(memrchr(s, '@', (e - s))))) {
    if ((pp = static_cast<const char *>(memchr(s, ':', (p - s))))) {
      result.set_value(string("user", 4), string(s, static_cast<string::size_type>(pp - s)));
      pp++;
      result.set_value(string("pass", 4), string(pp, static_cast<string::size_type>(p - pp)));
    } else {
      result.set_value(string("user", 4), string(s, static_cast<string::size_type>(p - s)));
    }

    s = p + 1;
  }

  /* check for port */
  if (s < ue && *s == '[' && *(e - 1) == ']') {
    /* Short circuit portscan, we're dealing with an IPv6 embedded address */
    p = nullptr;
  } else {
    p = static_cast<const char *>(memrchr(s, ':', (e - s)));
  }

  if (p) {
    if (!result.isset(string("port", 4))) {
      p++;
      if (e - p > 5) { /* port cannot be longer then 5 characters */
        return {};
      } else if (e - p > 0) {
        int64_t port = string(p, static_cast<string::size_type>(e - p)).to_int();
        if (port > 0 && port <= 65535) {
          result.set_value(string("port", 4), port);
        } else {
          return {};
        }
      }
      p--;
    }
  } else {
    p = e;
  }

  /* check if we have a valid host, if we don't reject the string as url */
  if ((p - s) < 1) {
    return {};
  }
  result.set_value(string("host", 4), string(s, static_cast<string::size_type>(p - s)));

  if (e == ue) {
    return result;
  }

  s = e;

just_path:

  e = ue;
  p = static_cast<const char *>(memchr(s, '#', (e - s)));
  if (p) {
    p++;
    if (p < e) {
      result.set_value(string("fragment", 8), string(p, static_cast<string::size_type>(e - p)));
    }
    e = p - 1;
  }

  p = static_cast<const char *>(memchr(s, '?', (e - s)));
  if (p) {
    p++;
    if (p < e) {
      result.set_value(string("query", 5), string(p, static_cast<string::size_type>(e - p)));
    }
    e = p - 1;
  }

  if (s < e || s == ue) {
    result.set_value(string("path", 4), string(s, static_cast<string::size_type>(e - s)));
  }

  return result;
}

} // namespace

string f$base64_encode(const string &s) noexcept {
  int32_t result_len = (s.size() + 2) / 3 * 4;
  string res(result_len, false);
  result_len = base64_encode(reinterpret_cast<const unsigned char *>(s.c_str()), static_cast<int32_t>(s.size()), res.buffer(), result_len + 1);

  if (result_len != 0) {
    return {};
  }

  return res;
}

/* Function f$base64_decode ported from https://github.com/php/php-src/blob/master/ext/standard/base64.c#L130
 * "This product includes PHP software, freely available from <http://www.php.net/software/>".
 */
Optional<string> f$base64_decode(const string &s, bool strict) noexcept {
  /* run through the whole string, converting as we go */
  string::size_type result_len = (s.size() + 3) / 4 * 3;
  string result(result_len, false);
  int32_t i = 0;
  string::size_type j = 0;
  int32_t padding = 0;
  for (string::size_type pos = 0; pos < s.size(); pos++) {
    int32_t ch = static_cast<unsigned char>(s[pos]);
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

string f$rawurlencode(const string &s) noexcept {
  auto &runtime_ctx{RuntimeContext::get()};
  runtime_ctx.static_SB.clean().reserve(3 * s.size());

  const auto &string_lib_constants{StringLibConstants::get()};
  for (string::size_type i = 0; i < s.size(); ++i) {
    if (good_url_symbols_mask[static_cast<unsigned char>(s[i])] == '1') {
      runtime_ctx.static_SB.append_char(s[i]);
    } else {
      runtime_ctx.static_SB.append_char('%');
      runtime_ctx.static_SB.append_char(string_lib_constants.uhex_digits[(s[i] >> 4) & 0x0F]);
      runtime_ctx.static_SB.append_char(string_lib_constants.uhex_digits[s[i] & 0x0F]);
    }
  }
  return runtime_ctx.static_SB.str();
}

string f$rawurldecode(const string &s) noexcept {
  auto &runtime_ctx{RuntimeContext::get()};
  runtime_ctx.static_SB.clean().reserve(s.size());

  for (string::size_type i = 0; i < s.size(); ++i) {
    if (s[i] == '%') {
      if (const uint32_t num_high{hex_to_int(s[i + 1])}; num_high < 16) {
        if (const uint32_t num_low{hex_to_int(s[i + 2])}; num_low < 16) {
          runtime_ctx.static_SB.append_char(static_cast<char>((num_high << 4) + num_low));
          i += 2;
          continue;
        }
      }
    }
    runtime_ctx.static_SB.append_char(s[i]);
  }
  return runtime_ctx.static_SB.str();
}

string f$urlencode(const string &s) noexcept {
  auto &runtime_ctx{RuntimeContext::get()};
  runtime_ctx.static_SB.clean().reserve(3 * s.size());

  const auto &string_lib_constants{StringLibConstants::get()};
  for (string::size_type i = 0; i < s.size(); ++i) {
    if (good_url_symbols_mask[static_cast<unsigned char>(s[i])] == '1') {
      runtime_ctx.static_SB.append_char(s[i]);
    } else if (s[i] == ' ') {
      runtime_ctx.static_SB.append_char('+');
    } else {
      runtime_ctx.static_SB.append_char('%');
      runtime_ctx.static_SB.append_char(string_lib_constants.uhex_digits[(s[i] >> 4) & 0x0F]);
      runtime_ctx.static_SB.append_char(string_lib_constants.uhex_digits[s[i] & 0x0F]);
    }
  }
  return runtime_ctx.static_SB.str();
}

string f$urldecode(const string &s) noexcept {
  auto &runtime_ctx{RuntimeContext::get()};
  runtime_ctx.static_SB.clean().reserve(s.size());

  for (string::size_type i = 0; i < s.size(); ++i) {
    if (s[i] == '%') {
      if (const uint32_t num_high{hex_to_int(s[i + 1])}; num_high < 16) {
        if (const uint32_t num_low{hex_to_int(s[i + 2])}; num_low < 16) {
          runtime_ctx.static_SB.append_char(static_cast<char>((num_high << 4) + num_low));
          i += 2;
          continue;
        }
      }
    } else if (s[i] == '+') {
      runtime_ctx.static_SB.append_char(' ');
      continue;
    }
    runtime_ctx.static_SB.append_char(s[i]);
  }
  return runtime_ctx.static_SB.str();
}

// returns var, as array|false|null (null if component doesn't exist)
mixed f$parse_url(const string &s, int64_t component) noexcept {
  array<mixed> url_as_array = php_url_parse_ex(s.c_str(), s.size());

  if (url_as_array.empty()) {
    return false;
  }

  if (component != -1) {
    switch (component) {
      case 0: // PHP_URL_SCHEME
        return url_as_array.get_value(string("scheme", 6));
      case 1: // PHP_URL_HOST
        return url_as_array.get_value(string("host", 4));
      case 2: // PHP_URL_PORT
        return url_as_array.get_value(string("port", 4));
      case 3: // PHP_URL_USER
        return url_as_array.get_value(string("user", 4));
      case 4: // PHP_URL_PASS
        return url_as_array.get_value(string("pass", 4));
      case 5: // PHP_URL_PATH
        return url_as_array.get_value(string("path", 4));
      case 6: // PHP_URL_QUERY
        return url_as_array.get_value(string("query", 5));
      case 7: // PHP_URL_FRAGMENT
        return url_as_array.get_value(string("fragment", 8));
      default:
        php_warning("Wrong parameter component = %" PRIi64 " in function parse_url", component);
        return false;
    }
  }

  return url_as_array;
}
