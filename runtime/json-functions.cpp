// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/json-functions.h"

#include "runtime/exception.h"
#include "runtime/string_functions.h"

static void json_append_one_char(unsigned int c) {
  static_SB.append_char('\\');
  static_SB.append_char('u');
  static_SB.append_char("0123456789abcdef"[c >> 12]);
  static_SB.append_char("0123456789abcdef"[(c >> 8) & 15]);
  static_SB.append_char("0123456789abcdef"[(c >> 4) & 15]);
  static_SB.append_char("0123456789abcdef"[c & 15]);
}

static bool json_append_char(unsigned int c) {
  if (c < 0x10000) {
    if (0xD7FF < c && c < 0xE000) {
      return false;
    }
    json_append_one_char(c);
    return true;
  } else if (c <= 0x10ffff) {
    c -= 0x10000;
    json_append_one_char(0xD800 | (c >> 10));
    json_append_one_char(0xDC00 | (c & 0x3FF));
    return true;
  } else {
    return false;
  }
}

static bool do_json_encode_string_php(const char *s, int len, int64_t options) {
  int begin_pos = static_SB.size();
  if (options & JSON_UNESCAPED_UNICODE) {
    static_SB.reserve(2 * len + 2);
  } else {
    static_SB.reserve(6 * len + 2);
  }
  static_SB.append_char('"');

#define ERROR {static_SB.set_pos (begin_pos); static_SB.append ("null", 4); return false;}
#define CHECK(x) if (!(x)) {php_warning ("Not a valid utf-8 character at pos %d in function json_encode", pos); ERROR}
#define APPEND_CHAR(x) CHECK(json_append_char (x))

  int a, b, c, d;
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
      case -128 ... -1:
        a = s[pos];
        CHECK ((a & 0x40) != 0);

        b = s[++pos];
        CHECK((b & 0xc0) == 0x80);
        if ((a & 0x20) == 0) {
          CHECK((a & 0x1e) > 0);
          if (options & JSON_UNESCAPED_UNICODE) {
            static_SB.append_char(static_cast<char>(a));
            static_SB.append_char(static_cast<char>(b));
          } else {
            APPEND_CHAR(((a & 0x1f) << 6) | (b & 0x3f));
          }
          break;
        }

        c = s[++pos];
        CHECK((c & 0xc0) == 0x80);
        if ((a & 0x10) == 0) {
          CHECK(((a & 0x0f) | (b & 0x20)) > 0);
          if (options & JSON_UNESCAPED_UNICODE) {
            static_SB.append_char(static_cast<char>(a));
            static_SB.append_char(static_cast<char>(b));
            static_SB.append_char(static_cast<char>(c));
          } else {
            APPEND_CHAR(((a & 0x0f) << 12) | ((b & 0x3f) << 6) | (c & 0x3f));
          }
          break;
        }

        d = s[++pos];
        CHECK((d & 0xc0) == 0x80);
        if ((a & 0x08) == 0) {
          CHECK(((a & 0x07) | (b & 0x30)) > 0);
          if (options & JSON_UNESCAPED_UNICODE) {
            static_SB.append_char(static_cast<char>(a));
            static_SB.append_char(static_cast<char>(b));
            static_SB.append_char(static_cast<char>(c));
            static_SB.append_char(static_cast<char>(d));
          } else {
            APPEND_CHAR(((a & 0x07) << 18) | ((b & 0x3f) << 12) | ((c & 0x3f) << 6) | (d & 0x3f));
          }
          break;
        }

        CHECK(0);
        break;
      default:
        static_SB.append_char(s[pos]);
        break;
    }
  }

  static_SB.append_char('"');
  return true;
#undef ERROR
#undef CHECK
#undef APPEND_CHAR
}

static bool do_json_encode_string_vkext(const char *s, int len) {
  static_SB.reserve(2 * len + 2);
  if (static_SB.string_buffer_error_flag == STRING_BUFFER_ERROR_FLAG_FAILED) {
    return false;
  }

  static_SB.append_char('"');

  for (int pos = 0; pos < len; pos++) {
    char c = s[pos];
    if (unlikely ((unsigned int)c < 32u)) {
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

bool do_json_encode(const mixed &v, int64_t options, bool simple_encode) {
  switch (v.get_type()) {
    case mixed::type::NUL:
      static_SB.append("null", 4);
      return true;
    case mixed::type::BOOLEAN:
      if (v.as_bool()) {
        static_SB.append("true", 4);
      } else {
        static_SB.append("false", 5);
      }
      return true;
    case mixed::type::INTEGER:
      static_SB << v.as_int();
      return true;
    case mixed::type::FLOAT:
      if (is_ok_float(v.as_double())) {
        static_SB << (simple_encode ? f$number_format(v.as_double(), 6, DOT, string()) : string(v.as_double()));
      } else {
        php_warning("strange double %lf in function json_encode", v.as_double());
        if (options & JSON_PARTIAL_OUTPUT_ON_ERROR) {
          static_SB.append("0", 1);
        } else {
          return false;
        }
      }
      return true;
    case mixed::type::STRING:
      if (simple_encode) {
        return do_json_encode_string_vkext(v.as_string().c_str(), v.as_string().size());
      }

      return do_json_encode_string_php(v.as_string().c_str(), v.as_string().size(), options);
    case mixed::type::ARRAY: {
      bool is_vector = v.as_array().is_vector();
      const bool force_object = static_cast<bool>(JSON_FORCE_OBJECT & options);
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
            if (!do_json_encode(key, options, simple_encode)) {
              if (!(options & JSON_PARTIAL_OUTPUT_ON_ERROR)) {
                return false;
              }
            }
          }
          static_SB << ':';
        }

        if (!do_json_encode(p.get_value(), options, simple_encode)) {
          if (!(options & JSON_PARTIAL_OUTPUT_ON_ERROR)) {
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

Optional<string> f$json_encode(const mixed &v, int64_t options, bool simple_encode) {
  bool has_unsupported_option = static_cast<bool>(options & ~JSON_AVAILABLE_OPTIONS);
  if (has_unsupported_option) {
    php_warning("Wrong parameter options = %" PRIi64 " in function json_encode", options);
    return false;
  }

  static_SB.clean();

  if (!do_json_encode(v, options, simple_encode)) {
    return false;
  }

  return static_SB.str();
}

string f$vk_json_encode_safe(const mixed &v, bool simple_encode) {
  static_SB.clean();
  string_buffer::string_buffer_error_flag = STRING_BUFFER_ERROR_FLAG_ON;
  do_json_encode(v, 0, simple_encode);
  if (string_buffer::string_buffer_error_flag == STRING_BUFFER_ERROR_FLAG_FAILED) {
    static_SB.clean();
    string_buffer::string_buffer_error_flag = STRING_BUFFER_ERROR_FLAG_OFF;
    THROW_EXCEPTION (new_Exception(string(__FILE__), __LINE__, string("json_encode buffer overflow", 27)));
    return string();
  }
  string_buffer::string_buffer_error_flag = STRING_BUFFER_ERROR_FLAG_OFF;
  return static_SB.str();
}


static void json_skip_blanks(const char *s, int &i) {
  while (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n') {
    i++;
  }
}

static bool do_json_decode(const char *s, int s_len, int &i, mixed &v) {
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
                    value[l] = (char)num;
                  } else if (num < 0x800) {
                    value[l++] = (char)(0xc0 + (num >> 6));
                    value[l] = (char)(0x80 + (num & 63));
                  } else if (num < 0xffff) {
                    value[l++] = (char)(0xe0 + (num >> 12));
                    value[l++] = (char)(0x80 + ((num >> 6) & 63));
                    value[l] = (char)(0x80 + (num & 63));
                  } else {
                    value[l++] = (char)(0xf0 + (num >> 18));
                    value[l++] = (char)(0x80 + ((num >> 12) & 63));
                    value[l++] = (char)(0x80 + ((num >> 6) & 63));
                    value[l] = (char)(0x80 + (num & 63));
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

mixed f$json_decode(const string &v, bool assoc) {
  if (!assoc) {
//    php_warning ("json_decode doesn't support decoding to class, returning array");
  }

  mixed result;
  int i = 0;
  if (do_json_decode(v.c_str(), v.size(), i, result)) {
    json_skip_blanks(v.c_str(), i);
    if (i == (int)v.size()) {
      return result;
    }
  }

  return mixed();
}
