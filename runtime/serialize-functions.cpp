// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/serialize-functions.h"

#include "runtime/context/runtime-context.h"

void impl_::PhpSerializer::serialize(bool b) noexcept {
  kphp_runtime_context.static_SB.reserve(4);
  kphp_runtime_context.static_SB.append_char('b');
  kphp_runtime_context.static_SB.append_char(':');
  kphp_runtime_context.static_SB.append_char(static_cast<char>(b + '0'));
  kphp_runtime_context.static_SB.append_char(';');
}

void impl_::PhpSerializer::serialize(int64_t i) noexcept {
  kphp_runtime_context.static_SB.reserve(24);
  kphp_runtime_context.static_SB.append_char('i');
  kphp_runtime_context.static_SB.append_char(':');
  kphp_runtime_context.static_SB << i;
  kphp_runtime_context.static_SB.append_char(';');
}

void impl_::PhpSerializer::serialize(double f) noexcept {
  kphp_runtime_context.static_SB.append("d:", 2);
  kphp_runtime_context.static_SB << f << ';';
}

void impl_::PhpSerializer::serialize(const string &s) noexcept {
  string::size_type len = s.size();
  kphp_runtime_context.static_SB.reserve(25 + len);
  kphp_runtime_context.static_SB.append_char('s');
  kphp_runtime_context.static_SB.append_char(':');
  kphp_runtime_context.static_SB << len;
  kphp_runtime_context.static_SB.append_char(':');
  kphp_runtime_context.static_SB.append_char('"');
  kphp_runtime_context.static_SB.append_unsafe(s.c_str(), len);
  kphp_runtime_context.static_SB.append_char('"');
  kphp_runtime_context.static_SB.append_char(';');
}

void impl_::PhpSerializer::serialize_null() noexcept {
  kphp_runtime_context.static_SB.reserve(2);
  kphp_runtime_context.static_SB.append_char('N');
  kphp_runtime_context.static_SB.append_char(';');
}

void impl_::PhpSerializer::serialize(const mixed &v) noexcept {
  switch (v.get_type()) {
    case mixed::type::NUL:
      return serialize_null();
    case mixed::type::BOOLEAN:
      return serialize(v.as_bool());
    case mixed::type::INTEGER:
      return serialize(v.as_int());
    case mixed::type::FLOAT:
      return serialize(v.as_double());
    case mixed::type::STRING:
      return serialize(v.as_string());
    case mixed::type::ARRAY:
      return serialize(v.as_array());
    default:
      __builtin_unreachable();
  }
}

namespace {

int do_unserialize(const char *s, int s_len, mixed &out_var_value) noexcept {
  if (!out_var_value.is_null()) {
    out_var_value = mixed{};
  }
  switch (s[0]) {
    case 'N':
      if (s[1] == ';') {
        return 2;
      }
      break;
    case 'b':
      if (s[1] == ':' && (static_cast<unsigned int>(s[2] - '0') < 2u) && s[3] == ';') {
        out_var_value = static_cast<bool>(s[2] - '0');
        return 4;
      }
      break;
    case 'd':
      if (s[1] == ':') {
        s += 2;
        char *end_ptr = nullptr;
        double floatval = strtod(s, &end_ptr);
        if (*end_ptr == ';' && end_ptr > s) {
          out_var_value = floatval;
          return static_cast<int>(end_ptr - s + 3);
        }
      }
      break;
    case 'i':
      if (s[1] == ':') {
        s += 2;
        int j = 0;
        while (s[j]) {
          if (s[j] == ';') {
            int64_t intval = 0;
            if (php_try_to_int(s, j, &intval)) {
              s += j + 1;
              out_var_value = intval;
              return j + 3;
            }

            int k = 0;
            if (s[k] == '-' || s[k] == '+') {
              k++;
            }
            while ('0' <= s[k] && s[k] <= '9') {
              k++;
            }
            if (k == j) {
              out_var_value = mixed(s, j);
              return j + 3;
            }
            return 0;
          }
          j++;
        }
      }
      break;
    case 's':
      if (s[1] == ':') {
        s += 2;
        string::size_type j = 0;
        string::size_type len = 0;
        while ('0' <= s[j] && s[j] <= '9') {
          len = len * 10 + s[j++] - '0';
        }
        if (j > 0 && s[j] == ':' && s[j + 1] == '"' && len < string::max_size() && j + 2 + len < s_len) {
          s += j + 2;

          if (s[len] == '"' && s[len + 1] == ';') {
            out_var_value = mixed(s, len);
            return len + 6 + j;
          }
        }
      }
      break;
    case 'a':
      if (s[1] == ':') {
        const char *s_begin = s;
        s += 2;
        int j = 0;
        int len = 0;
        while ('0' <= s[j] && s[j] <= '9') {
          len = len * 10 + s[j++] - '0';
        }
        if (j > 0 && len >= 0 && s[j] == ':' && s[j + 1] == '{') {
          s += j + 2;
          s_len -= j + 4;

          array_size size(len, false);
          if (s[0] == 'i') {//try to cheat
            size = array_size(len, s[1] == ':' && s[2] == '0' && s[3] == ';');
          }
          array<mixed> res(size);

          while (len-- > 0) {
            if (s[0] == 'i' && s[1] == ':') {
              s += 2;
              int k = 0;
              while (s[k]) {
                if (s[k] == ';') {
                  int64_t intval = 0;
                  if (php_try_to_int(s, k, &intval)) {
                    s += k + 1;
                    s_len -= k + 3;
                    int length = do_unserialize(s, s_len, res[intval]);
                    if (!length) {
                      return 0;
                    }
                    s += length;
                    s_len -= length;
                    break;
                  }

                  int q = 0;
                  if (s[q] == '-' || s[q] == '+') {
                    q++;
                  }
                  while ('0' <= s[q] && s[q] <= '9') {
                    q++;
                  }
                  if (q == k) {
                    string key(s, k);
                    s += k + 1;
                    s_len -= k + 3;
                    int length = do_unserialize(s, s_len, res[key]);
                    if (!length) {
                      return 0;
                    }
                    s += length;
                    s_len -= length;
                    break;
                  }
                  return 0;
                }
                k++;
              }
            } else if (s[0] == 's' && s[1] == ':') {
              s += 2;
              string::size_type k = 0;
              string::size_type str_len = 0;
              while ('0' <= s[k] && s[k] <= '9') {
                str_len = str_len * 10 + s[k++] - '0';
              }
              if (k > 0 && s[k] == ':' && s[k + 1] == '"' && str_len < string::max_size() && k + 2 + str_len < s_len) {
                s += k + 2;

                if (s[str_len] == '"' && s[str_len + 1] == ';') {
                  string key(s, str_len);
                  s += str_len + 2;
                  s_len -= str_len + 6 + k;
                  int length = do_unserialize(s, s_len, res[key]);
                  if (!length) {
                    return 0;
                  }
                  s += length;
                  s_len -= length;
                }
              } else {
                return 0;
              }
            } else {
              return 0;
            }
          }

          if (s[0] == '}') {
            out_var_value = std::move(res);
            return static_cast<int>(s - s_begin + 1);
          }
        }
      }
      break;
  }
  return 0;
}

} // namespace

mixed unserialize_raw(const char *v, int32_t v_len) noexcept {
  mixed result;

  if (do_unserialize(v, v_len, result) == v_len) {
    return result;
  }

  return false;
}

mixed f$unserialize(const string &v) noexcept {
  return unserialize_raw(v.c_str(), v.size());
}
