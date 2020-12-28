// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/serialize-functions.h"

static inline void do_serialize(bool b) {
  static_SB.reserve(4);
  static_SB.append_char('b');
  static_SB.append_char(':');
  static_SB.append_char(static_cast<char>(b + '0'));
  static_SB.append_char(';');
}

static inline void do_serialize(int64_t i) {
  static_SB.reserve(24);
  static_SB.append_char('i');
  static_SB.append_char(':');
  static_SB << i;
  static_SB.append_char(';');
}

static inline void do_serialize(double f) {
  static_SB.append("d:", 2);
  static_SB << f << ';';
}

static inline void do_serialize(const string &s) {
  string::size_type len = s.size();
  static_SB.reserve(25 + len);
  static_SB.append_char('s');
  static_SB.append_char(':');
  static_SB << len;
  static_SB.append_char(':');
  static_SB.append_char('"');
  static_SB.append_unsafe(s.c_str(), len);
  static_SB.append_char('"');
  static_SB.append_char(';');
}

void do_serialize(const mixed &v) {
  switch (v.get_type()) {
    case mixed::type::NUL:
      static_SB.reserve(2);
      static_SB.append_char('N');
      static_SB.append_char(';');
      return;
    case mixed::type::BOOLEAN:
      return do_serialize(v.as_bool());
    case mixed::type::INTEGER:
      return do_serialize(v.as_int());
    case mixed::type::FLOAT:
      return do_serialize(v.as_double());
    case mixed::type::STRING:
      return do_serialize(v.as_string());
    case mixed::type::ARRAY: {
      static_SB.append("a:", 2);
      static_SB << v.as_array().count();
      static_SB.append(":{", 2);
      for (array<mixed>::const_iterator p = v.as_array().begin(); p != v.as_array().end(); ++p) {
        const array<mixed>::key_type &key = p.get_key();
        if (array<mixed>::is_int_key(key)) {
          do_serialize(key.to_int());
        } else {
          do_serialize(key.to_string());
        }
        do_serialize(p.get_value());
      }
      static_SB << '}';
      return;
    }
    default:
      __builtin_unreachable();
  }
}

string f$serialize(const mixed &v) {
  static_SB.clean();

  do_serialize(v);

  return static_SB.str();
}

static int do_unserialize(const char *s, int s_len, mixed &out_var_value) {
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
      if (s[1] == ':' && ((unsigned int)(s[2] - '0') < 2u) && s[3] == ';') {
        out_var_value = static_cast<bool>(s[2] - '0');
        return 4;
      }
      break;
    case 'd':
      if (s[1] == ':') {
        s += 2;
        char *end_ptr;
        double floatval = strtod(s, &end_ptr);
        if (*end_ptr == ';' && end_ptr > s) {
          out_var_value = floatval;
          return (int)(end_ptr - s + 3);
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
        uint32_t j = 0;
        uint32_t len = 0;
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
        int j = 0, len = 0;
        while ('0' <= s[j] && s[j] <= '9') {
          len = len * 10 + s[j++] - '0';
        }
        if (j > 0 && len >= 0 && s[j] == ':' && s[j + 1] == '{') {
          s += j + 2;
          s_len -= j + 4;

          array_size size(0, len, false);
          if (s[0] == 'i') {//try to cheat
            size = array_size(len, 0, s[1] == ':' && s[2] == '0' && s[3] == ';');
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
              uint32_t k = 0;
              uint32_t str_len = 0;
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
            return (int)(s - s_begin + 1);
          }
        }
      }
      break;
  }
  return 0;
}

mixed unserialize_raw(const char *v, int32_t v_len) {
  mixed result;

  if (do_unserialize(v, v_len, result) == v_len) {
    return result;
  }

  return false;
}


mixed f$unserialize(const string &v) {
  return unserialize_raw(v.c_str(), v.size());
}
