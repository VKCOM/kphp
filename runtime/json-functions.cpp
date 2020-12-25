// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/json-functions.h"

#include "common/algorithms/find.h"

#include "runtime/exception.h"
#include "runtime/string_functions.h"

namespace {

void json_append_one_char(unsigned int c) noexcept {
  static_SB.append_char('\\');
  static_SB.append_char('u');
  static_SB.append_char("0123456789abcdef"[c >> 12]);
  static_SB.append_char("0123456789abcdef"[(c >> 8) & 15]);
  static_SB.append_char("0123456789abcdef"[(c >> 4) & 15]);
  static_SB.append_char("0123456789abcdef"[c & 15]);
}

bool json_append_char(unsigned int c) noexcept {
  if (c < 0x10000) {
    if (0xD7FF < c && c < 0xE000) {
      return false;
    }
    json_append_one_char(c);
    return true;
  }
  if (c <= 0x10ffff) {
    c -= 0x10000;
    json_append_one_char(0xD800 | (c >> 10));
    json_append_one_char(0xDC00 | (c & 0x3FF));
    return true;
  }
  return false;
}


bool do_json_encode_string_php(const char *s, int len, int64_t options) noexcept {
  int begin_pos = static_SB.size();
  if (options & JSON_UNESCAPED_UNICODE) {
    static_SB.reserve(2 * len + 2);
  } else {
    static_SB.reserve(6 * len + 2);
  }
  static_SB.append_char('"');

  auto fire_error = [begin_pos](int pos) {
    php_warning("Not a valid utf-8 character at pos %d in function json_encode", pos);
    static_SB.set_pos(begin_pos);
    static_SB.append("null", 4);
    return false;
  };

  for (int pos = 0; pos < len; pos++) {
    switch (s[pos]) {
      case '"':
        static_SB.append_char('\\');
        static_SB.append_char('"');
        break;
      case '\\':
        static_SB.append_char('\\');
        static_SB.append_char('\\');
        break;
      case '/':
        static_SB.append_char('\\');
        static_SB.append_char('/');
        break;
      case '\b':
        static_SB.append_char('\\');
        static_SB.append_char('b');
        break;
      case '\f':
        static_SB.append_char('\\');
        static_SB.append_char('f');
        break;
      case '\n':
        static_SB.append_char('\\');
        static_SB.append_char('n');
        break;
      case '\r':
        static_SB.append_char('\\');
        static_SB.append_char('r');
        break;
      case '\t':
        static_SB.append_char('\\');
        static_SB.append_char('t');
        break;
      case 0 ... 7:
      case 11:
      case 14 ... 31:
        json_append_one_char(s[pos]);
        break;
      case -128 ... -1: {
        const int a = s[pos];
        if ((a & 0x40) == 0) {
          return fire_error(pos);
        }

        const int b = s[++pos];
        if ((b & 0xc0) != 0x80) {
          return fire_error(pos);
        }
        if ((a & 0x20) == 0) {
          if ((a & 0x1e) <= 0) {
            return fire_error(pos);
          }
          if (options & JSON_UNESCAPED_UNICODE) {
            static_SB.append_char(static_cast<char>(a));
            static_SB.append_char(static_cast<char>(b));
          } else if (!json_append_char(((a & 0x1f) << 6) | (b & 0x3f))) {
            return fire_error(pos);
          }
          break;
        }

        const int c = s[++pos];
        if ((c & 0xc0) != 0x80) {
          return fire_error(pos);
        }
        if ((a & 0x10) == 0) {
          if (((a & 0x0f) | (b & 0x20)) <= 0) {
            return fire_error(pos);
          }
          if (options & JSON_UNESCAPED_UNICODE) {
            static_SB.append_char(static_cast<char>(a));
            static_SB.append_char(static_cast<char>(b));
            static_SB.append_char(static_cast<char>(c));
          } else if (!json_append_char(((a & 0x0f) << 12) | ((b & 0x3f) << 6) | (c & 0x3f))) {
            return fire_error(pos);
          }
          break;
        }

        const int d = s[++pos];
        if ((d & 0xc0) != 0x80) {
          return fire_error(pos);
        }
        if ((a & 0x08) == 0) {
          if (((a & 0x07) | (b & 0x30)) <= 0) {
            return fire_error(pos);
          }
          if (options & JSON_UNESCAPED_UNICODE) {
            static_SB.append_char(static_cast<char>(a));
            static_SB.append_char(static_cast<char>(b));
            static_SB.append_char(static_cast<char>(c));
            static_SB.append_char(static_cast<char>(d));
          } else if (!json_append_char(((a & 0x07) << 18) | ((b & 0x3f) << 12) | ((c & 0x3f) << 6) | (d & 0x3f))) {
            return fire_error(pos);
          }
          break;
        }

        return fire_error(pos);
      }
      default:
        static_SB.append_char(s[pos]);
        break;
    }
  }

  static_SB.append_char('"');
  return true;
}

bool do_json_encode_string_vkext(const char *s, int len) noexcept {
  static_SB.reserve(2 * len + 2);
  if (static_SB.string_buffer_error_flag == STRING_BUFFER_ERROR_FLAG_FAILED) {
    return false;
  }

  static_SB.append_char('"');

  for (int pos = 0; pos < len; pos++) {
    char c = s[pos];
    if (unlikely (static_cast<unsigned int>(c) < 32u)) {
      switch (c) {
        case '\b':
          static_SB.append_char('\\');
          static_SB.append_char('b');
          break;
        case '\f':
          static_SB.append_char('\\');
          static_SB.append_char('f');
          break;
        case '\n':
          static_SB.append_char('\\');
          static_SB.append_char('n');
          break;
        case '\r':
          static_SB.append_char('\\');
          static_SB.append_char('r');
          break;
        case '\t':
          static_SB.append_char('\\');
          static_SB.append_char('t');
          break;
      }
    } else {
      if (c == '"' || c == '\\' || c == '/') {
        static_SB.append_char('\\');
      }
      static_SB.append_char(c);
    }
  }

  static_SB.append_char('"');

  return true;
}

} // namespace

namespace impl_ {

JsonEncoder::JsonEncoder(int64_t options, bool simple_encode) noexcept:
  options_(options),
  simple_encode_(simple_encode) {
}

bool JsonEncoder::encode(bool b) noexcept {
  if (b) {
    static_SB.append("true", 4);
  } else {
    static_SB.append("false", 5);
  }
  return true;
}

bool JsonEncoder::encode_null() noexcept {
  static_SB.append("null", 4);
  return true;
}

bool JsonEncoder::encode(int64_t i) noexcept {
  static_SB << i;
  return true;
}

bool JsonEncoder::encode(double d) noexcept {
  if (is_ok_float(d)) {
    static_SB << (simple_encode_ ? f$number_format(d, 6, string{"."}, string()) : string(d));
  } else {
    php_warning("strange double %lf in function json_encode", d);
    if (options_ & JSON_PARTIAL_OUTPUT_ON_ERROR) {
      static_SB.append("0", 1);
    } else {
      return false;
    }
  }
  return true;
}

bool JsonEncoder::encode(const string &s) noexcept {
  return simple_encode_ ? do_json_encode_string_vkext(s.c_str(), s.size()) : do_json_encode_string_php(s.c_str(), s.size(), options_);
}

bool JsonEncoder::encode(const mixed &v) noexcept {
  switch (v.get_type()) {
    case mixed::type::NUL:
      return encode_null();
    case mixed::type::BOOLEAN:
      return encode(v.as_bool());
    case mixed::type::INTEGER:
      return encode(v.as_int());
    case mixed::type::FLOAT:
      return encode(v.as_double());
    case mixed::type::STRING:
      return encode(v.as_string());
    case mixed::type::ARRAY: {
      bool is_vector = v.as_array().is_vector();
      const bool force_object = static_cast<bool>(JSON_FORCE_OBJECT & options_);
      if (!force_object && !is_vector && v.as_array().size().string_size == 0) {
        int n = 0;
        for (array<mixed>::const_iterator p = v.as_array().begin(); p != v.as_array().end(); ++p) {
          if (p.get_key().to_int() != n) {
            break;
          }
          n++;
        }
        if (n == v.as_array().count()) {
          if (v.as_array().get_next_key() == v.as_array().count()) {
            is_vector = true;
          } else {
            php_warning("Corner case in json conversion, [] could be easy transformed to {}");
          }
        }
      }
      is_vector &= !force_object;

      static_SB << "{["[is_vector];

      for (array<mixed>::const_iterator p = v.as_array().begin(); p != v.as_array().end(); ++p) {
        if (p != v.as_array().begin()) {
          static_SB << ',';
        }

        if (!is_vector) {
          const array<mixed>::key_type key = p.get_key();
          if (array<mixed>::is_int_key(key)) {
            static_SB << '"' << key.to_int() << '"';
          } else {
            if (!encode(key)) {
              if (!(options_ & JSON_PARTIAL_OUTPUT_ON_ERROR)) {
                return false;
              }
            }
          }
          static_SB << ':';
        }

        if (!encode(p.get_value())) {
          if (!(options_ & JSON_PARTIAL_OUTPUT_ON_ERROR)) {
            return false;
          }
        }
      }

      static_SB << "}]"[is_vector];
      return true;
    }
    default:
      __builtin_unreachable();
  }
}


} // namespace impl_

namespace {

void json_skip_blanks(const char *s, int &i) noexcept {
  while (vk::any_of_equal(s[i], ' ', '\t', '\r', '\n')) {
    i++;
  }
}

bool do_json_decode(const char *s, int s_len, int &i, mixed &v) noexcept {
  if (!v.is_null()) {
    v.destroy();
  }
  json_skip_blanks(s, i);
  switch (s[i]) {
    case 'n':
      if (s[i + 1] == 'u' &&
          s[i + 2] == 'l' &&
          s[i + 3] == 'l') {
        i += 4;
        return true;
      }
      break;
    case 't':
      if (s[i + 1] == 'r' &&
          s[i + 2] == 'u' &&
          s[i + 3] == 'e') {
        i += 4;
        new(&v) mixed(true);
        return true;
      }
      break;
    case 'f':
      if (s[i + 1] == 'a' &&
          s[i + 2] == 'l' &&
          s[i + 3] == 's' &&
          s[i + 4] == 'e') {
        i += 5;
        new(&v) mixed(false);
        return true;
      }
      break;
    case '"': {
      int j = i + 1;
      int slashes = 0;
      while (j < s_len && s[j] != '"') {
        if (s[j] == '\\') {
          slashes++;
          j++;
        }
        j++;
      }
      if (j < s_len) {
        int len = j - i - 1 - slashes;

        string value(len, false);

        i++;
        int l;
        for (l = 0; l < len && i < j; l++) {
          char c = s[i];
          if (c == '\\') {
            i++;
            switch (s[i]) {
              case '"':
              case '\\':
              case '/':
                value[l] = s[i];
                break;
              case 'b':
                value[l] = '\b';
                break;
              case 'f':
                value[l] = '\f';
                break;
              case 'n':
                value[l] = '\n';
                break;
              case 'r':
                value[l] = '\r';
                break;
              case 't':
                value[l] = '\t';
                break;
              case 'u':
                if (isxdigit(s[i + 1]) && isxdigit(s[i + 2]) && isxdigit(s[i + 3]) && isxdigit(s[i + 4])) {
                  int num = 0;
                  for (int t = 0; t < 4; t++) {
                    char c = s[++i];
                    if ('0' <= c && c <= '9') {
                      num = num * 16 + c - '0';
                    } else {
                      c |= 0x20;
                      if ('a' <= c && c <= 'f') {
                        num = num * 16 + c - 'a' + 10;
                      }
                    }
                  }

                  if (0xD7FF < num && num < 0xE000) {
                    if (s[i + 1] == '\\' && s[i + 2] == 'u' &&
                        isxdigit(s[i + 3]) && isxdigit(s[i + 4]) && isxdigit(s[i + 5]) && isxdigit(s[i + 6])) {
                      i += 2;
                      int u = 0;
                      for (int t = 0; t < 4; t++) {
                        char c = s[++i];
                        if ('0' <= c && c <= '9') {
                          u = u * 16 + c - '0';
                        } else {
                          c |= 0x20;
                          if ('a' <= c && c <= 'f') {
                            u = u * 16 + c - 'a' + 10;
                          }
                        }
                      }

                      if (0xD7FF < u && u < 0xE000) {
                        num = (((num & 0x3FF) << 10) | (u & 0x3FF)) + 0x10000;
                      } else {
                        i -= 6;
                        return false;
                      }
                    } else {
                      return false;
                    }
                  }

                  if (num < 128) {
                    value[l] = static_cast<char>(num);
                  } else if (num < 0x800) {
                    value[l++] = static_cast<char>(0xc0 + (num >> 6));
                    value[l] = static_cast<char>(0x80 + (num & 63));
                  } else if (num < 0xffff) {
                    value[l++] = static_cast<char>(0xe0 + (num >> 12));
                    value[l++] = static_cast<char>(0x80 + ((num >> 6) & 63));
                    value[l] = static_cast<char>(0x80 + (num & 63));
                  } else {
                    value[l++] = static_cast<char>(0xf0 + (num >> 18));
                    value[l++] = static_cast<char>(0x80 + ((num >> 12) & 63));
                    value[l++] = static_cast<char>(0x80 + ((num >> 6) & 63));
                    value[l] = static_cast<char>(0x80 + (num & 63));
                  }
                  break;
                }
                /* fallthrough */
              default:
                return false;
            }
            i++;
          } else {
            value[l] = s[i++];
          }
        }
        value.shrink(l);

        new(&v) mixed(value);
        i++;
        return true;
      }
      break;
    }
    case '[': {
      array<mixed> res;
      i++;
      json_skip_blanks(s, i);
      if (s[i] != ']') {
        do {
          mixed value;
          if (!do_json_decode(s, s_len, i, value)) {
            return false;
          }
          res.push_back(value);
          json_skip_blanks(s, i);
        } while (s[i++] == ',');

        if (s[i - 1] != ']') {
          return false;
        }
      } else {
        i++;
      }

      new(&v) mixed(res);
      return true;
    }
    case '{': {
      array<mixed> res;
      i++;
      json_skip_blanks(s, i);
      if (s[i] != '}') {
        do {
          mixed key;
          if (!do_json_decode(s, s_len, i, key) || !key.is_string()) {
            return false;
          }
          json_skip_blanks(s, i);
          if (s[i++] != ':') {
            return false;
          }

          if (!do_json_decode(s, s_len, i, res[key])) {
            return false;
          }
          json_skip_blanks(s, i);
        } while (s[i++] == ',');

        if (s[i - 1] != '}') {
          return false;
        }
      } else {
        i++;
      }

      new(&v) mixed(res);
      return true;
    }
    default: {
      int j = i;
      while (s[j] == '-' || ('0' <= s[j] && s[j] <= '9') || s[j] == 'e' || s[j] == 'E' || s[j] == '+' || s[j] == '.') {
        j++;
      }
      if (j > i) {
        int64_t intval = 0;
        if (php_try_to_int(s + i, j - i, &intval)) {
          i = j;
          new(&v) mixed(intval);
          return true;
        }

        char *end_ptr;
        double floatval = strtod(s + i, &end_ptr);
        if (end_ptr == s + j) {
          i = j;
          new(&v) mixed(floatval);
          return true;
        }
      }
      break;
    }
  }

  return false;
}

} // namespace

mixed f$json_decode(const string &v, bool assoc) noexcept {
  static_cast<void>(assoc);

  mixed result;
  int i = 0;
  if (do_json_decode(v.c_str(), v.size(), i, result)) {
    json_skip_blanks(v.c_str(), i);
    if (i == static_cast<int>(v.size())) {
      return result;
    }
  }

  return mixed();
}
