// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/kphp_core.h"

template<class LongT>
class LongNumber {
  inline void convert_from(const string &s) {
    static const char *php_class_name = std::is_same<LongT, unsigned long long>{} ? "ULong"  :
                                        std::is_same<LongT,          long long>{} ? "Long"   :
                                        std::is_same<LongT, unsigned       int>{} ? "UInt32" :
                                        "LongNumber";

    if (s[0] == '0' && s[1] == 'x') {
      l = 0;
      const string::size_type len = s.size();
      string::size_type cur = 2;
      while (cur < len) {
        char c = s[cur];
        if ('0' <= c && c <= '9') {
          l = l * 16 + c - '0';
        } else {
          c |= 0x20;
          if ('a' <= c && c <= 'f') {
            l = l * 16 + c - 'a' + 10;
          } else {
            break;
          }
        }
        cur++;
      }

      constexpr size_t cnt_bits_in_hex_digit = 4;
      size_t cnt_hex_digits = strlen("0x") + std::numeric_limits<LongT>::digits / cnt_bits_in_hex_digit;
      if (len == 2 || cur < len || cnt_hex_digits < len) {
        php_warning("Wrong conversion from hexadecimal string \"%s\" to %s", s.c_str(), php_class_name);
      }
      return;
    }

    bool need_warning = false;
    int mul = 1;
    int len = s.size();
    int cur = 0;
    if (len > 0 && (s[0] == '-' || s[0] == '+')) {
      if (s[0] == '-') {
        mul = -1;
        need_warning = !std::numeric_limits<LongT>::is_signed;
      }
      cur++;
    }
    while (cur + 1 < len && s[cur] == '0') {
      cur++;
    }

    int cnt_decimal_digits = std::numeric_limits<LongT>::digits10 + 1;
    need_warning |= (cur == len || len - cur > cnt_decimal_digits);
    l = 0;
    while ('0' <= s[cur] && s[cur] <= '9') {
      l = l * 10 + (s[cur++] - '0');
    }
    // TODO: -9223372036854775808 doesn't work here :(
    if (need_warning || cur < len || l % 10 != static_cast<LongT>(s[len - 1] - '0')) {
      php_warning("Wrong conversion from string \"%s\" to %s", s.c_str(), php_class_name);
    }
    l *= mul;
  }

public:
  LongT l{0};

  inline LongNumber() = default;

  inline LongNumber(LongT l) :
    l(static_cast<LongT>(l)) {
  }

  inline explicit LongNumber(int64_t i) :
    l(static_cast<LongT>(i)) {
  }

  inline explicit LongNumber(double d) :
    l(static_cast<LongT>(d)) {
  }

  inline explicit LongNumber(const string &s) {
    convert_from(s);
  }

  inline explicit LongNumber(const mixed &v) {
    switch (v.get_type()) {
      case mixed::type::INTEGER: {
        l = static_cast<LongT>(v.as_int());
        break;
      }
      case mixed::type::STRING: {
        convert_from(v.as_string());
        break;
      }
      case mixed::type::FLOAT: {
        l = static_cast<LongT>(v.as_double());
        break;
      }
      case mixed::type::NUL: {
        l = 0;
        break;
      }
      case mixed::type::BOOLEAN: {
        l = static_cast<LongT>(v.as_bool());
        break;
      }
      default: {
        l = static_cast<LongT>(v.to_int());
      }
    }
  }
};

inline Long f$labs(Long lhs) {
  return Long(lhs.l < 0 ? -lhs.l : lhs.l);
}

inline Long f$ldiv(Long lhs, Long rhs) {
  if (rhs.l == 0) {
    php_warning("Long division by zero");
    return 0;
  }

  return Long(lhs.l / rhs.l);
}

inline Long f$lmod(Long lhs, Long rhs) {
  if (rhs.l == 0) {
    php_warning("Long modulo by zero");
    return 0;
  }

  return Long(lhs.l % rhs.l);
}

inline Long f$lpow(Long lhs, int64_t deg) {
  long long result = 1;
  long long mul = lhs.l;

  while (deg > 0) {
    if (deg & 1) {
      result *= mul;
    }
    mul *= mul;
    deg >>= 1;
  }

  return Long(result);
}

inline Long f$ladd(Long lhs, Long rhs) {
  return Long(lhs.l + rhs.l);
}

inline Long f$lsub(Long lhs, Long rhs) {
  return Long(lhs.l - rhs.l);
}

inline Long f$lmul(Long lhs, Long rhs) {
  return Long(lhs.l * rhs.l);
}

inline Long f$lshl(Long lhs, int64_t rhs) {
  return Long(lhs.l << rhs);
}

inline Long f$lshr(Long lhs, int64_t rhs) {
  return Long(lhs.l >> rhs);
}

inline Long f$lnot(Long lhs) {
  return Long(~lhs.l);
}

inline Long f$lor(Long lhs, Long rhs) {
  return Long(lhs.l | rhs.l);
}

inline Long f$land(Long lhs, Long rhs) {
  return Long(lhs.l & rhs.l);
}

inline Long f$lxor(Long lhs, Long rhs) {
  return Long(lhs.l ^ rhs.l);
}

inline int64_t f$lcomp(Long lhs, Long rhs) {
  if (lhs.l < rhs.l) {
    return -1;
  }
  return lhs.l > rhs.l;
}

inline Long f$longval(long long val) {
  return Long(val);
}

inline Long f$longval(bool val) {
  return Long(static_cast<long long>(val));
}

inline Long f$longval(int64_t val) {
  return Long(val);
}

inline Long f$longval(double val) {
  return Long(val);
}

inline Long f$longval(const string &val) {
  return Long(val);
}

inline Long f$longval(const mixed &val) {
  return Long(val);
}

inline bool f$boolval(Long val) {
  return val.l;
}

inline int64_t f$intval(Long val) {
  return static_cast<int64_t>(val.l);
}

inline int64_t f$safe_intval(Long val) {
  return static_cast<int64_t>(val.l);
}

inline double f$floatval(Long val) {
  return static_cast<double>(val.l);
}

inline string f$strval(Long val) {
  char buf[20];
  return string{buf, static_cast<string::size_type>(simd_int64_to_string(val.l, buf) - buf)};
}

inline string_buffer &operator<<(string_buffer &buf, Long x) {
  return buf << x.l;
}

inline ULong f$uldiv(ULong lhs, ULong rhs) {
  if (rhs.l == 0) {
    php_warning("ULong division by zero");
    return 0;
  }

  return ULong(lhs.l / rhs.l);
}

inline ULong f$ulmod(ULong lhs, ULong rhs) {
  if (rhs.l == 0) {
    php_warning("ULong modulo by zero");
    return 0;
  }

  return ULong(lhs.l % rhs.l);
}

inline ULong f$ulpow(ULong lhs, int64_t deg) {
  unsigned long long result = 1;
  unsigned long long mul = lhs.l;

  while (deg > 0) {
    if (deg & 1) {
      result *= mul;
    }
    mul *= mul;
    deg >>= 1;
  }

  return ULong(result);
}

inline ULong f$uladd(ULong lhs, ULong rhs) {
  return ULong(lhs.l + rhs.l);
}

inline ULong f$ulsub(ULong lhs, ULong rhs) {
  return ULong(lhs.l - rhs.l);
}

inline ULong f$ulmul(ULong lhs, ULong rhs) {
  return ULong(lhs.l * rhs.l);
}

inline ULong f$ulshl(ULong lhs, int64_t rhs) {
  return ULong(lhs.l << rhs);
}

inline ULong f$ulshr(ULong lhs, int64_t rhs) {
  return ULong(lhs.l >> rhs);
}

inline ULong f$ulnot(ULong lhs) {
  return ULong(~lhs.l);
}

inline ULong f$ulor(ULong lhs, ULong rhs) {
  return ULong(lhs.l | rhs.l);
}

inline ULong f$uland(ULong lhs, ULong rhs) {
  return ULong(lhs.l & rhs.l);
}

inline ULong f$ulxor(ULong lhs, ULong rhs) {
  return ULong(lhs.l ^ rhs.l);
}

inline int64_t f$ulcomp(ULong lhs, ULong rhs) {
  if (lhs.l < rhs.l) {
    return -1;
  }
  return lhs.l > rhs.l;
}

inline ULong f$ulongval(unsigned long long val) {
  return ULong(val);
}

inline ULong f$ulongval(bool val) {
  return ULong(static_cast<unsigned long long>(val));
}

inline ULong f$ulongval(int64_t val) {
  return ULong(val);
}

inline ULong f$ulongval(double val) {
  return ULong(val);
}

inline ULong f$ulongval(const string &val) {
  return ULong(val);
}

inline ULong f$ulongval(const mixed &val) {
  return ULong(val);
}

inline bool f$boolval(ULong val) {
  return static_cast<bool>(val.l);
}

inline int64_t f$intval(ULong val) {
  return static_cast<int64_t>(val.l);
}

inline int64_t f$safe_intval(ULong val) {
  if (val.l >= static_cast<unsigned long long int>(std::numeric_limits<int64_t>::max())) {
    php_warning("Integer overflow on converting %llu to int", val.l);
  }
  return static_cast<int64_t>(val.l);
}

inline double f$floatval(ULong val) {
  return static_cast<double>(val.l);
}

inline string f$strval(ULong val) {
  char buf[20];
  return string{buf, static_cast<string::size_type>(simd_uint64_to_string(val.l, buf) - buf)};
}

inline string_buffer &operator<<(string_buffer &buf, ULong x) {
  return buf << x.l;
}

inline UInt f$uidiv(UInt lhs, UInt rhs) {
  if (rhs.l == 0) {
    php_warning("UInt division by zero");
    return 0;
  }

  return UInt(lhs.l / rhs.l);
}

inline UInt f$uimod(UInt lhs, UInt rhs) {
  if (rhs.l == 0) {
    php_warning("UInt modulo by zero");
    return 0;
  }

  return UInt(lhs.l % rhs.l);
}

inline UInt f$uipow(UInt lhs, int64_t deg) {
  unsigned int result = 1;
  unsigned int mul = lhs.l;

  while (deg > 0) {
    if (deg & 1) {
      result *= mul;
    }
    mul *= mul;
    deg >>= 1;
  }

  return UInt(result);
}

inline UInt f$uiadd(UInt lhs, UInt rhs) {
  return UInt(lhs.l + rhs.l);
}

inline UInt f$uisub(UInt lhs, UInt rhs) {
  return UInt(lhs.l - rhs.l);
}

inline UInt f$uimul(UInt lhs, UInt rhs) {
  return UInt(lhs.l * rhs.l);
}

inline UInt f$uishl(UInt lhs, int64_t rhs) {
  return UInt(lhs.l << rhs);
}

inline UInt f$uishr(UInt lhs, int64_t rhs) {
  return UInt(lhs.l >> rhs);
}

inline UInt f$uinot(UInt lhs) {
  return UInt(~lhs.l);
}

inline UInt f$uior(UInt lhs, UInt rhs) {
  return UInt(lhs.l | rhs.l);
}

inline UInt f$uiand(UInt lhs, UInt rhs) {
  return UInt(lhs.l & rhs.l);
}

inline UInt f$uixor(UInt lhs, UInt rhs) {
  return UInt(lhs.l ^ rhs.l);
}

inline int64_t f$uicomp(UInt lhs, UInt rhs) {
  if (lhs.l < rhs.l) {
    return -1;
  }
  return lhs.l > rhs.l;
}

inline UInt f$uintval(unsigned int val) {
  return UInt(val);
}

inline UInt f$uintval(bool val) {
  return UInt(static_cast<unsigned int>(val));
}

inline UInt f$uintval(int64_t val) {
  return UInt(val);
}

inline UInt f$uintval(double val) {
  return UInt(val);
}

inline UInt f$uintval(const string &val) {
  return UInt(val);
}

inline UInt f$uintval(const mixed &val) {
  return UInt(val);
}

inline bool f$boolval(UInt val) {
  return val.l;
}

inline int64_t f$intval(UInt val) {
  return val.l;
}

inline int64_t f$safe_intval(UInt val) {
  return val.l;
}

inline double f$floatval(UInt val) {
  return val.l;
}

inline string f$strval(UInt val) {
  char buf[20];
  return string{buf, static_cast<string::size_type>(simd_uint32_to_string(val.l, buf) - buf)};
}

inline string_buffer &operator<<(string_buffer &buf, UInt x) {
  return buf << x.l;
}

inline const Long &f$longval(const Long &val) {
  return val;
}

inline Long f$longval(ULong val) {
  return (long long)val.l;
}

inline Long f$longval(UInt val) {
  return (long long)val.l;
}

inline ULong f$ulongval(Long val) {
  return (unsigned long long)val.l;
}

inline const ULong &f$ulongval(const ULong &val) {
  return val;
}

inline ULong f$ulongval(UInt val) {
  return (unsigned long long)val.l;
}

inline const UInt &f$uintval(const UInt &val) {
  return val;
}

inline UInt f$uintval(Long val) {
  return (unsigned int)val.l;
}

inline UInt f$uintval(ULong val) {
  return (unsigned int)val.l;
}
