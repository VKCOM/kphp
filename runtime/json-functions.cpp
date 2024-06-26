// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/json-functions.h"

#include "common/algorithms/find.h"

#include "runtime/exception.h"
#include "runtime/string_functions.h"

// note: json-functions.cpp is used for non-typed json implementation: for json_encode() and json_decode()
// for classes, e.g. `JsonEncoder::encode(new A)`, see json-writer.cpp and from/to visitors
namespace {

void json_append_one_char(unsigned int c) noexcept {
  kphp_runtime_context.static_SB.append_char('\\');
  kphp_runtime_context.static_SB.append_char('u');
  kphp_runtime_context.static_SB.append_char("0123456789abcdef"[c >> 12]);
  kphp_runtime_context.static_SB.append_char("0123456789abcdef"[(c >> 8) & 15]);
  kphp_runtime_context.static_SB.append_char("0123456789abcdef"[(c >> 4) & 15]);
  kphp_runtime_context.static_SB.append_char("0123456789abcdef"[c & 15]);
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


bool do_json_encode_string_php(const JsonPath &json_path, const char *s, int len, int64_t options) noexcept {
  int begin_pos = kphp_runtime_context.static_SB.size();
  if (options & JSON_UNESCAPED_UNICODE) {
    kphp_runtime_context.static_SB.reserve(2 * len + 2);
  } else {
    kphp_runtime_context.static_SB.reserve(6 * len + 2);
  }
  kphp_runtime_context.static_SB.append_char('"');

  auto fire_error = [json_path, begin_pos](int pos) {
    php_warning("%s: Not a valid utf-8 character at pos %d in function json_encode", json_path.to_string().c_str(), pos);
    kphp_runtime_context.static_SB.set_pos(begin_pos);
    kphp_runtime_context.static_SB.append("null", 4);
    return false;
  };

  for (int pos = 0; pos < len; pos++) {
    switch (s[pos]) {
      case '"':
        kphp_runtime_context.static_SB.append_char('\\');
        kphp_runtime_context.static_SB.append_char('"');
        break;
      case '\\':
        kphp_runtime_context.static_SB.append_char('\\');
        kphp_runtime_context.static_SB.append_char('\\');
        break;
      case '/':
        kphp_runtime_context.static_SB.append_char('\\');
        kphp_runtime_context.static_SB.append_char('/');
        break;
      case '\b':
        kphp_runtime_context.static_SB.append_char('\\');
        kphp_runtime_context.static_SB.append_char('b');
        break;
      case '\f':
        kphp_runtime_context.static_SB.append_char('\\');
        kphp_runtime_context.static_SB.append_char('f');
        break;
      case '\n':
        kphp_runtime_context.static_SB.append_char('\\');
        kphp_runtime_context.static_SB.append_char('n');
        break;
      case '\r':
        kphp_runtime_context.static_SB.append_char('\\');
        kphp_runtime_context.static_SB.append_char('r');
        break;
      case '\t':
        kphp_runtime_context.static_SB.append_char('\\');
        kphp_runtime_context.static_SB.append_char('t');
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
            kphp_runtime_context.static_SB.append_char(static_cast<char>(a));
            kphp_runtime_context.static_SB.append_char(static_cast<char>(b));
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
            kphp_runtime_context.static_SB.append_char(static_cast<char>(a));
            kphp_runtime_context.static_SB.append_char(static_cast<char>(b));
            kphp_runtime_context.static_SB.append_char(static_cast<char>(c));
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
            kphp_runtime_context.static_SB.append_char(static_cast<char>(a));
            kphp_runtime_context.static_SB.append_char(static_cast<char>(b));
            kphp_runtime_context.static_SB.append_char(static_cast<char>(c));
            kphp_runtime_context.static_SB.append_char(static_cast<char>(d));
          } else if (!json_append_char(((a & 0x07) << 18) | ((b & 0x3f) << 12) | ((c & 0x3f) << 6) | (d & 0x3f))) {
            return fire_error(pos);
          }
          break;
        }

        return fire_error(pos);
      }
      default:
        kphp_runtime_context.static_SB.append_char(s[pos]);
        break;
    }
  }

  kphp_runtime_context.static_SB.append_char('"');
  return true;
}

bool do_json_encode_string_vkext(const char *s, int len) noexcept {
  kphp_runtime_context.static_SB.reserve(2 * len + 2);
  if (kphp_runtime_context.sb_lib_context.error_flag == STRING_BUFFER_ERROR_FLAG_FAILED) {
    return false;
  }

  kphp_runtime_context.static_SB.append_char('"');

  for (int pos = 0; pos < len; pos++) {
    char c = s[pos];
    if (unlikely (static_cast<unsigned int>(c) < 32u)) {
      switch (c) {
        case '\b':
          kphp_runtime_context.static_SB.append_char('\\');
          kphp_runtime_context.static_SB.append_char('b');
          break;
        case '\f':
          kphp_runtime_context.static_SB.append_char('\\');
          kphp_runtime_context.static_SB.append_char('f');
          break;
        case '\n':
          kphp_runtime_context.static_SB.append_char('\\');
          kphp_runtime_context.static_SB.append_char('n');
          break;
        case '\r':
          kphp_runtime_context.static_SB.append_char('\\');
          kphp_runtime_context.static_SB.append_char('r');
          break;
        case '\t':
          kphp_runtime_context.static_SB.append_char('\\');
          kphp_runtime_context.static_SB.append_char('t');
          break;
      }
    } else {
      if (c == '"' || c == '\\' || c == '/') {
        kphp_runtime_context.static_SB.append_char('\\');
      }
      kphp_runtime_context.static_SB.append_char(c);
    }
  }

  kphp_runtime_context.static_SB.append_char('"');

  return true;
}

} // namespace

string JsonPath::to_string() const {
  // this function is called only when error is occurred, so it's not
  // very performance-sensitive

  if (depth == 0) {
    return string{"/", 1};
  }
  unsigned num_parts = std::clamp(depth, 0U, static_cast<unsigned>(arr.size()));
  string result;
  result.reserve_at_least((num_parts+1) * 8);
  result.push_back('/');
  for (unsigned i = 0; i < num_parts; i++) {
    const char *key = arr[i];
    if (key == nullptr) {
      // int key indexing
      result.append("[.]");
    } else {
      // string key indexing
      result.append("['");
      result.append(arr[i]);
      result.append("']");
    }
  }
  if (depth >= arr.size()) {
    result.append("...");
  }
  return result;
}

namespace impl_ {

JsonEncoder::JsonEncoder(int64_t options, bool simple_encode, const char *json_obj_magic_key) noexcept:
  options_(options),
  simple_encode_(simple_encode),
  json_obj_magic_key_(json_obj_magic_key) {
}

bool JsonEncoder::encode(bool b) noexcept {
  if (b) {
    kphp_runtime_context.static_SB.append("true", 4);
  } else {
    kphp_runtime_context.static_SB.append("false", 5);
  }
  return true;
}

bool JsonEncoder::encode_null() const noexcept {
  kphp_runtime_context.static_SB.append("null", 4);
  return true;
}

bool JsonEncoder::encode(int64_t i) noexcept {
  kphp_runtime_context.static_SB << i;
  return true;
}

bool JsonEncoder::encode(double d) noexcept {
  if (vk::any_of_equal(std::fpclassify(d), FP_INFINITE, FP_NAN)) {
    php_warning("%s: strange double %lf in function json_encode", json_path_.to_string().c_str(), d);
    if (options_ & JSON_PARTIAL_OUTPUT_ON_ERROR) {
      kphp_runtime_context.static_SB.append("0", 1);
    } else {
      return false;
    }
  } else {
    kphp_runtime_context.static_SB << (simple_encode_ ? f$number_format(d, 6, string{"."}, string{}) : string{d});
  }
  return true;
}

bool JsonEncoder::encode(const string &s) noexcept {
  return simple_encode_ ? do_json_encode_string_vkext(s.c_str(), s.size()) : do_json_encode_string_php(json_path_, s.c_str(), s.size(), options_);
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
    case mixed::type::ARRAY:
      return encode(v.as_array());
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

bool do_json_decode(const char *s, int s_len, int &i, mixed &v, const char *json_obj_magic_key) noexcept {
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
          if (!do_json_decode(s, s_len, i, value, json_obj_magic_key)) {
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
          if (!do_json_decode(s, s_len, i, key, json_obj_magic_key) || !key.is_string()) {
            return false;
          }
          json_skip_blanks(s, i);
          if (s[i++] != ':') {
            return false;
          }

          if (!do_json_decode(s, s_len, i, res[key], json_obj_magic_key)) {
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

      // it's impossible to distinguish whether empty php array was an json array or json object;
      // to overcome it we add dummy key to php array that make array::is_vector() returning false, so we have difference
      if (json_obj_magic_key && res.empty()) {
        res[string{json_obj_magic_key}] = true;
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

std::pair<mixed, bool> json_decode(const string &v, const char *json_obj_magic_key) noexcept {
  mixed result;
  int i = 0;
  if (do_json_decode(v.c_str(), v.size(), i, result, json_obj_magic_key)) {
    json_skip_blanks(v.c_str(), i);
    if (i == static_cast<int>(v.size())) {
      bool success = true;
      return {result, success};
    }
  }

  return {};
}

mixed f$json_decode(const string &v, bool assoc) noexcept {
  // TODO It was a warning before (in case if assoc is false), but then it was disabled, should we enable it again?
  static_cast<void>(assoc);
  return json_decode(v).first;
}
