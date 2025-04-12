// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/string/mbstring-functions.h"

#include "common/unicode/unicode-utils.h"
#include "common/unicode/utf8-utils.h"
#include "runtime-common/stdlib/string/string-functions.h"

int mb_detect_encoding(const string &encoding) noexcept {
  const auto *encoding_name = f$strtolower(encoding).c_str();

  if (!strcmp(encoding_name, "cp1251") || !strcmp(encoding_name, "cp-1251") || !strcmp(encoding_name, "windows-1251")) {
    return 1251;
  }

  if (!strcmp(encoding_name, "utf8") || !strcmp(encoding_name, "utf-8")) {
    return 8;
  }

  return -1;
}

int64_t mb_UTF8_strlen(const char *s) {
  int64_t res = 0;
  for (int64_t i = 0; s[i]; i++) {
    if (((static_cast<unsigned char>(s[i])) & 0xc0) != 0x80) {
      res++;
    }
  }
  return res;
}

int64_t mb_UTF8_advance(const char *s, int64_t cnt) {
  php_assert(cnt >= 0);
  int64_t i = 0;
  for (i = 0; s[i] && cnt >= 0; i++) {
    if (((static_cast<unsigned char>(s[i])) & 0xc0) != 0x80) {
      cnt--;
    }
  }
  if (cnt < 0) {
    i--;
  }
  return i;
}

int64_t mb_UTF8_get_offset(const char *s, int64_t pos) {
  int64_t res = 0;
  for (int64_t i = 0; i < pos && s[i]; i++) {
    if (((static_cast<unsigned char>(s[i])) & 0xc0) != 0x80) {
      res++;
    }
  }
  return res;
}

bool mb_UTF8_check(const char *s) noexcept {
  do {
#define CHECK(condition)                                                                                                                                       \
  if (!(condition)) {                                                                                                                                          \
    return false;                                                                                                                                              \
  }
    unsigned int a = static_cast<unsigned char>(*s++);
    if ((a & 0x80) == 0) {
      if (a == 0) {
        return true;
      }
      continue;
    }

    CHECK((a & 0x40) != 0);

    unsigned int b = static_cast<unsigned char>(*s++);
    CHECK((b & 0xc0) == 0x80);
    if ((a & 0x20) == 0) {
      CHECK((a & 0x1e) > 0);
      continue;
    }

    unsigned int c = static_cast<unsigned char>(*s++);
    CHECK((c & 0xc0) == 0x80);
    if ((a & 0x10) == 0) {
      int x = (((a & 0x0f) << 6) | (b & 0x20));
      CHECK(x != 0 && x != 0x360); // surrogates
      continue;
    }

    unsigned int d = static_cast<unsigned char>(*s++);
    CHECK((d & 0xc0) == 0x80);
    if ((a & 0x08) == 0) {
      int t = (((a & 0x07) << 6) | (b & 0x30));
      CHECK(0 < t && t < 0x110); // end of unicode
      continue;
    }

    return false;
#undef CHECK
  } while (true);

  php_assert(0);
}

bool f$mb_check_encoding(const string &str, const string &encoding) noexcept {
  int encoding_num = mb_detect_encoding(encoding);
  if (encoding_num < 0) {
    php_critical_error("encoding \"%s\" doesn't supported in mb_check_encoding", encoding.c_str());
    return !str.empty();
  }

  if (encoding_num == 1251) {
    return true;
  }

  return mb_UTF8_check(str.c_str());
}

int64_t f$mb_strlen(const string &str, const string &encoding) noexcept {
  int encoding_num = mb_detect_encoding(encoding);
  if (encoding_num < 0) {
    php_critical_error("encoding \"%s\" doesn't supported in mb_strlen", encoding.c_str());
    return str.size();
  }

  if (encoding_num == 1251) {
    return str.size();
  }

  return mb_UTF8_strlen(str.c_str());
}

string f$mb_strtolower(const string &str, const string &encoding) noexcept {
  int encoding_num = mb_detect_encoding(encoding);
  if (encoding_num < 0) {
    php_critical_error("encoding \"%s\" doesn't supported in mb_strtolower", encoding.c_str());
    return str;
  }

  int len = str.size();
  if (encoding_num == 1251) {
    string res(len, false);
    for (int i = 0; i < len; i++) {
      switch (static_cast<unsigned char>(str[i])) {
        case 'A' ... 'Z':
          res[i] = static_cast<char>(str[i] + 'a' - 'A');
          break;
        case 0xC0 ... 0xDF:
          res[i] = static_cast<char>(str[i] + 32);
          break;
        case 0x81:
          res[i] = static_cast<char>(0x83);
          break;
        case 0xA3:
          res[i] = static_cast<char>(0xBC);
          break;
        case 0xA5:
          res[i] = static_cast<char>(0xB4);
          break;
        case 0xA1:
        case 0xB2:
        case 0xBD:
          res[i] = static_cast<char>(str[i] + 1);
          break;
        case 0x80:
        case 0x8A:
        case 0x8C ... 0x8F:
        case 0xA8:
        case 0xAA:
        case 0xAF:
          res[i] = static_cast<char>(str[i] + 16);
          break;
        default:
          res[i] = str[i];
      }
    }

    return res;
  } else {
    string res(len * 3, false);
    const char *s = str.c_str();
    int res_len = 0;
    int p = 0;
    int ch = 0;
    while ((p = get_char_utf8(&ch, s)) > 0) {
      s += p;
      res_len += put_char_utf8(unicode_tolower(ch), &res[res_len]);
    }
    if (p < 0) {
      php_warning("Incorrect UTF-8 string \"%s\" in function mb_strtolower", str.c_str());
    }
    res.shrink(res_len);

    return res;
  }
}

string f$mb_strtoupper(const string &str, const string &encoding) noexcept {
  int encoding_num = mb_detect_encoding(encoding);
  if (encoding_num < 0) {
    php_critical_error("encoding \"%s\" doesn't supported in mb_strtoupper", encoding.c_str());
    return str;
  }

  int len = str.size();
  if (encoding_num == 1251) {
    string res(len, false);
    for (int i = 0; i < len; i++) {
      switch (static_cast<unsigned char>(str[i])) {
        case 'a' ... 'z':
          res[i] = static_cast<char>(str[i] + 'A' - 'a');
          break;
        case 0xE0 ... 0xFF:
          res[i] = static_cast<char>(str[i] - 32);
          break;
        case 0x83:
          res[i] = static_cast<char>(0x81);
          break;
        case 0xBC:
          res[i] = static_cast<char>(0xA3);
          break;
        case 0xB4:
          res[i] = static_cast<char>(0xA5);
          break;
        case 0xA2:
        case 0xB3:
        case 0xBE:
          res[i] = static_cast<char>(str[i] - 1);
          break;
        case 0x98:
        case 0xA0:
        case 0xAD:
          res[i] = ' ';
          break;
        case 0x90:
        case 0x9A:
        case 0x9C ... 0x9F:
        case 0xB8:
        case 0xBA:
        case 0xBF:
          res[i] = static_cast<char>(str[i] - 16);
          break;
        default:
          res[i] = str[i];
      }
    }

    return res;
  } else {
    string res(len * 3, false);
    const char *s = str.c_str();
    int res_len = 0;
    int p = 0;
    int ch = 0;
    while ((p = get_char_utf8(&ch, s)) > 0) {
      s += p;
      res_len += put_char_utf8(unicode_toupper(ch), &res[res_len]);
    }
    if (p < 0) {
      php_warning("Incorrect UTF-8 string \"%s\" in function mb_strtoupper", str.c_str());
    }
    res.shrink(res_len);

    return res;
  }
}

namespace {

int check_strpos_agrs(const char *func_name, const string &needle, int64_t offset, const string &encoding) noexcept {
  if (unlikely(offset < 0)) {
    php_warning("Wrong offset = %" PRIi64 " in function %s()", offset, func_name);
    return 0;
  }
  if (unlikely(needle.empty())) {
    php_warning("Parameter needle is empty in function %s()", func_name);
    return 0;
  }

  const int encoding_num = mb_detect_encoding(encoding);
  if (unlikely(encoding_num < 0)) {
    php_critical_error("encoding \"%s\" doesn't supported in %s()", encoding.c_str(), func_name);
    return 0;
  }
  return encoding_num;
}

Optional<int64_t> mp_strpos_impl(const string &haystack, const string &needle, int64_t offset, int encoding_num) noexcept {
  if (encoding_num == 1251) {
    return f$strpos(haystack, needle, offset);
  }

  int64_t UTF8_offset = mb_UTF8_advance(haystack.c_str(), offset);
  const char *s = static_cast<const char *>(memmem(haystack.c_str() + UTF8_offset, haystack.size() - UTF8_offset, needle.c_str(), needle.size()));
  if (unlikely(s == nullptr)) {
    return false;
  }
  return mb_UTF8_get_offset(haystack.c_str() + UTF8_offset, s - (haystack.c_str() + UTF8_offset)) + offset;
}

} // namespace

Optional<int64_t> f$mb_strpos(const string &haystack, const string &needle, int64_t offset, const string &encoding) noexcept {
  if (const int encoding_num = check_strpos_agrs("mb_strpos", needle, offset, encoding)) {
    return mp_strpos_impl(haystack, needle, offset, encoding_num);
  }
  return false;
}

Optional<int64_t> f$mb_stripos(const string &haystack, const string &needle, int64_t offset, const string &encoding) noexcept {
  if (const int encoding_num = check_strpos_agrs("mb_stripos", needle, offset, encoding)) {
    return mp_strpos_impl(f$mb_strtolower(haystack, encoding), f$mb_strtolower(needle, encoding), offset, encoding_num);
  }
  return false;
}

string f$mb_substr(const string &str, int64_t start, const mixed &length_var, const string &encoding) noexcept {
  int encoding_num = mb_detect_encoding(encoding);
  if (encoding_num < 0) {
    php_critical_error("encoding \"%s\" doesn't supported in mb_substr", encoding.c_str());
    return str;
  }

  int64_t length = 0;
  if (length_var.is_null()) {
    length = std::numeric_limits<int64_t>::max();
  } else {
    length = length_var.to_int();
  }

  if (encoding_num == 1251) {
    Optional<string> res = f$substr(str, start, length);
    if (!res.has_value()) {
      return {};
    }
    return res.val();
  }

  int64_t len = mb_UTF8_strlen(str.c_str());
  if (start < 0) {
    start += len;
  }
  if (start > len) {
    start = len;
  }
  if (length < 0) {
    length = len - start + length;
  }
  if (length <= 0 || start < 0) {
    return {};
  }
  if (len - start < length) {
    length = len - start;
  }

  int64_t UTF8_start = mb_UTF8_advance(str.c_str(), start);
  int64_t UTF8_length = mb_UTF8_advance(str.c_str() + UTF8_start, length);

  return {str.c_str() + UTF8_start, static_cast<string::size_type>(UTF8_length)};
}
