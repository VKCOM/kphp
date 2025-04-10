// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/serialization/json-functions.h"

#include "common/algorithms/find.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/string/string-functions.h"

// note: json-functions.cpp is used for non-typed json implementation: for json_encode() and json_decode()
// for classes, e.g. `JsonEncoder::encode(new A)`, see json-writer.cpp and from/to visitors
namespace {

void json_append_one_char(unsigned int c, string_buffer& sb) noexcept {
  sb.append_char('\\');
  sb.append_char('u');
  sb.append_char("0123456789abcdef"[c >> 12]);
  sb.append_char("0123456789abcdef"[(c >> 8) & 15]);
  sb.append_char("0123456789abcdef"[(c >> 4) & 15]);
  sb.append_char("0123456789abcdef"[c & 15]);
}

bool json_append_char(unsigned int c, string_buffer& sb) noexcept {
  if (c < 0x10000) {
    if (0xD7FF < c && c < 0xE000) {
      return false;
    }
    json_append_one_char(c, sb);
    return true;
  }
  if (c <= 0x10ffff) {
    c -= 0x10000;
    json_append_one_char(0xD800 | (c >> 10), sb);
    json_append_one_char(0xDC00 | (c & 0x3FF), sb);
    return true;
  }
  return false;
}

bool do_json_encode_string_php(const JsonPath& json_path, const char* s, int len, int64_t options, string_buffer& sb) noexcept {
  int begin_pos = sb.size();
  if (options & JSON_UNESCAPED_UNICODE) {
    sb.reserve(2 * len + 2);
  } else {
    sb.reserve(6 * len + 2);
  }
  sb.append_char('"');

  auto fire_error = [json_path, begin_pos, &sb](int pos) {
    php_warning("%s: Not a valid utf-8 character at pos %d in function json_encode", json_path.to_string().c_str(), pos);
    sb.set_pos(begin_pos);
    sb.append("null", 4);
    return false;
  };

  for (int pos = 0; pos < len; pos++) {
    switch (s[pos]) {
    case '"':
      sb.append_char('\\');
      sb.append_char('"');
      break;
    case '\\':
      sb.append_char('\\');
      sb.append_char('\\');
      break;
    case '/':
      sb.append_char('\\');
      sb.append_char('/');
      break;
    case '\b':
      sb.append_char('\\');
      sb.append_char('b');
      break;
    case '\f':
      sb.append_char('\\');
      sb.append_char('f');
      break;
    case '\n':
      sb.append_char('\\');
      sb.append_char('n');
      break;
    case '\r':
      sb.append_char('\\');
      sb.append_char('r');
      break;
    case '\t':
      sb.append_char('\\');
      sb.append_char('t');
      break;
    case 0 ... 7:
    case 11:
    case 14 ... 31:
      json_append_one_char(s[pos], sb);
      break;
    case -128 ... - 1: {
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
          sb.append_char(static_cast<char>(a));
          sb.append_char(static_cast<char>(b));
        } else if (!json_append_char(((a & 0x1f) << 6) | (b & 0x3f), sb)) {
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
          sb.append_char(static_cast<char>(a));
          sb.append_char(static_cast<char>(b));
          sb.append_char(static_cast<char>(c));
        } else if (!json_append_char(((a & 0x0f) << 12) | ((b & 0x3f) << 6) | (c & 0x3f), sb)) {
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
          sb.append_char(static_cast<char>(a));
          sb.append_char(static_cast<char>(b));
          sb.append_char(static_cast<char>(c));
          sb.append_char(static_cast<char>(d));
        } else if (!json_append_char(((a & 0x07) << 18) | ((b & 0x3f) << 12) | ((c & 0x3f) << 6) | (d & 0x3f), sb)) {
          return fire_error(pos);
        }
        break;
      }

      return fire_error(pos);
    }
    default:
      sb.append_char(s[pos]);
      break;
    }
  }

  sb.append_char('"');
  return true;
}

bool do_json_encode_string_vkext(const char* s, int len, string_buffer& sb) noexcept {
  auto& ctx = RuntimeContext::get(); // dirty hack :(
  sb.reserve(2 * len + 2);
  if (ctx.sb_lib_context.error_flag == STRING_BUFFER_ERROR_FLAG_FAILED) {
    return false;
  }

  sb.append_char('"');

  for (int pos = 0; pos < len; pos++) {
    char c = s[pos];
    if (unlikely(static_cast<unsigned int>(c) < 32U)) {
      switch (c) {
      case '\b':
        sb.append_char('\\');
        sb.append_char('b');
        break;
      case '\f':
        sb.append_char('\\');
        sb.append_char('f');
        break;
      case '\n':
        sb.append_char('\\');
        sb.append_char('n');
        break;
      case '\r':
        sb.append_char('\\');
        sb.append_char('r');
        break;
      case '\t':
        sb.append_char('\\');
        sb.append_char('t');
        break;
      default:
        break;
      }
    } else {
      if (c == '"' || c == '\\' || c == '/') {
        sb.append_char('\\');
      }
      sb.append_char(c);
    }
  }

  sb.append_char('"');

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
  result.reserve_at_least((num_parts + 1) * 8);
  result.push_back('/');
  for (unsigned i = 0; i < num_parts; i++) {
    const char* key = arr[i];
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

JsonEncoder::JsonEncoder(int64_t options, bool simple_encode, const char* json_obj_magic_key) noexcept
    : options_(options),
      simple_encode_(simple_encode),
      json_obj_magic_key_(json_obj_magic_key) {}

bool JsonEncoder::encode(bool b, string_buffer& sb) noexcept {
  if (b) {
    sb.append("true", 4);
  } else {
    sb.append("false", 5);
  }
  return true;
}

bool JsonEncoder::encode_null(string_buffer& sb) const noexcept {
  sb.append("null", 4);
  return true;
}

bool JsonEncoder::encode(int64_t i, string_buffer& sb) noexcept {
  sb << i;
  return true;
}

bool JsonEncoder::encode(double d, string_buffer& sb) noexcept {
  if (vk::any_of_equal(std::fpclassify(d), FP_INFINITE, FP_NAN)) {
    php_warning("%s: strange double %lf in function json_encode", json_path_.to_string().c_str(), d);
    if (options_ & JSON_PARTIAL_OUTPUT_ON_ERROR) {
      sb.append("0", 1);
    } else {
      return false;
    }
  } else {
    sb << (simple_encode_ ? f$number_format(d, 6, string{"."}, string{}) : string{d});
  }
  return true;
}

bool JsonEncoder::encode(const string& s, string_buffer& sb) noexcept {
  return simple_encode_ ? do_json_encode_string_vkext(s.c_str(), s.size(), sb) : do_json_encode_string_php(json_path_, s.c_str(), s.size(), options_, sb);
}

bool JsonEncoder::encode(const mixed& v, string_buffer& sb) noexcept {
  switch (v.get_type()) {
  case mixed::type::NUL:
    return encode_null(sb);
  case mixed::type::BOOLEAN:
    return encode(v.as_bool(), sb);
  case mixed::type::INTEGER:
    return encode(v.as_int(), sb);
  case mixed::type::FLOAT:
    return encode(v.as_double(), sb);
  case mixed::type::STRING:
    return encode(v.as_string(), sb);
  case mixed::type::ARRAY:
    return encode(v.as_array(), sb);
  default:
    __builtin_unreachable();
  }
}

} // namespace impl_

namespace {

void json_skip_blanks(const char* s, int& i) noexcept {
  while (vk::any_of_equal(s[i], ' ', '\t', '\r', '\n')) {
    i++;
  }
}

bool do_json_decode(const char* s, int s_len, int& i, mixed& v, const char* json_obj_magic_key) noexcept {
  if (!v.is_null()) {
    v.destroy();
  }
  json_skip_blanks(s, i);
  switch (s[i]) {
  case 'n':
    if (s[i + 1] == 'u' && s[i + 2] == 'l' && s[i + 3] == 'l') {
      i += 4;
      return true;
    }
    break;
  case 't':
    if (s[i + 1] == 'r' && s[i + 2] == 'u' && s[i + 3] == 'e') {
      i += 4;
      new (&v) mixed(true);
      return true;
    }
    break;
  case 'f':
    if (s[i + 1] == 'a' && s[i + 2] == 'l' && s[i + 3] == 's' && s[i + 4] == 'e') {
      i += 5;
      new (&v) mixed(false);
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
                if (s[i + 1] == '\\' && s[i + 2] == 'u' && isxdigit(s[i + 3]) && isxdigit(s[i + 4]) && isxdigit(s[i + 5]) && isxdigit(s[i + 6])) {
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

      new (&v) mixed(value);
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

    new (&v) mixed(res);
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

    new (&v) mixed(res);
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
        new (&v) mixed(intval);
        return true;
      }

      char* end_ptr;
      double floatval = strtod(s + i, &end_ptr);
      if (end_ptr == s + j) {
        i = j;
        new (&v) mixed(floatval);
        return true;
      }
    }
    break;
  }
  }

  return false;
}

} // namespace

std::pair<mixed, bool> json_decode(const string& v, const char* json_obj_magic_key) noexcept {
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

mixed f$json_decode(const string& v, bool assoc) noexcept {
  // TODO It was a warning before (in case if assoc is false), but then it was disabled, should we enable it again?
  static_cast<void>(assoc);
  return json_decode(v).first;
}
