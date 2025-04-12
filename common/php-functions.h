// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <climits>
#include <cstring>
#include <cctype>
#include <limits>
#include <type_traits>

#include "common/sanitizer.h"

constexpr int STRLEN_WARNING_FLAG = 1 << 30;
constexpr int STRLEN_OBJECT = -3;
constexpr int STRLEN_ERROR = -2;
constexpr int STRLEN_DYNAMIC = -1;
constexpr int STRLEN_UNKNOWN = STRLEN_ERROR;
constexpr int STRLEN_EMPTY = 0;
constexpr int STRLEN_BOOL = 1;
constexpr int STRLEN_BOOL_ = STRLEN_BOOL;
constexpr int STRLEN_INT32 = 11;
constexpr int STRLEN_INT64 = 20;
constexpr int STRLEN_INT = STRLEN_INT64;
constexpr int STRLEN_FLOAT = 21;
constexpr int STRLEN_ARRAY = 5;
constexpr int STRLEN_ARRAY_ = STRLEN_ARRAY | STRLEN_WARNING_FLAG;
constexpr int STRLEN_STRING = STRLEN_DYNAMIC;
constexpr int STRLEN_VAR = STRLEN_DYNAMIC;
constexpr int STRLEN_CLASS = STRLEN_DYNAMIC;
constexpr int STRLEN_VOID = STRLEN_ERROR;
constexpr int STRLEN_FUTURE = STRLEN_ERROR;
constexpr int STRLEN_FUTURE_QUEUE = STRLEN_ERROR;

constexpr int SIZEOF_STRING = 8;
constexpr int SIZEOF_ARRAY_ANY = 8;
constexpr int SIZEOF_MIXED = 16;
constexpr int SIZEOF_INSTANCE_ANY = 8;
constexpr int SIZEOF_OPTIONAL = 8;
constexpr int SIZEOF_FUTURE = 8;
constexpr int SIZEOF_FUTURE_QUEUE = 8;
constexpr int SIZEOF_REGEXP = 48;
constexpr int SIZEOF_UNKNOWN = 1;

class ExtraRefCnt {
public:
  enum extra_ref_cnt_value {
    // used as a reference counter for global constants,
    // which are usually used as a default initialization of various variables;
    // data is located in heap or data section and is readonly
    for_global_const = 0x7ffffff0,

    // used as a reference counter for instance_cache variables;
    // data is located in shared memory;
    // recursive array deletion is implied
    for_instance_cache,

    // used as a reference counter for the confdata variables;
    // data is located in shared memory;
    // either recursive or non-recursive array deletion is implied (depends on the context)
    for_confdata,

    // used as a reference counter for communication with job workers
    // data is located in shared memory;
    // hard reset without the destructor calls is implied
    for_job_worker_communication
  };

  // wrapping this into a class helps avoid the global namespace pollution
  ExtraRefCnt(extra_ref_cnt_value value) noexcept:
    value_(value) {}

  operator int() const noexcept {
    return static_cast<int>(value_);
  }

private:
  extra_ref_cnt_value value_;
};

inline int64_t string_hash(const char *p, size_t l) __attribute__ ((always_inline)) ubsan_supp("alignment");

int64_t string_hash(const char *p, size_t l) {
  constexpr uint64_t HASH_MUL = 1915239017;
  uint64_t hash = 2147483648U;

  size_t prev = (l & 3);
  for (size_t i = 0; i < prev; i++) {
    hash = hash * HASH_MUL + p[i];
  }

  const auto *p_uint = reinterpret_cast<const uint32_t *>(p + prev);
  l >>= 2;
  while (l-- > 0) {
    hash = hash * HASH_MUL + *p_uint++;
  }
  const auto result = static_cast<int64_t>(hash);
  // to ensure that there is no way to get the -9223372036854775808L during code generation
  return (result != std::numeric_limits<int64_t>::min()) * result;
}

inline bool php_is_numeric(const char *s) {
  while (isspace(*s)) {
    s++;
  }

  if (*s == '+' || *s == '-') {
    s++;
  }

  int l = 0;
  while (*s >= '0' && *s <= '9') {
    l++;
    s++;
  }

  if (*s == '.') {
    s++;
    while (*s >= '0' && *s <= '9') {
      l++;
      s++;
    }
  }

  if (l == 0) {
    return false;
  }

  if (*s == 'e' || *s == 'E') {
    s++;
    if (*s == '+' || *s == '-') {
      s++;
    }

    if (*s == '\0') {
      return false;
    }

    while (*s >= '0' && *s <= '9') {
      s++;
    }
  }

  return *s == '\0';
}

inline bool php_is_int(const char *s, size_t l) __attribute__ ((always_inline));

bool php_is_int(const char *s, size_t l) {
  if (l == 0) {
    return false;
  }
  const uint8_t has_minus = s[0] == '-';
  if (has_minus || s[0] == '+') {
    s++;
    l--;

    if (s[0] < '1' || s[0] > '9') {
      return false;
    }
  } else {
    if (s[0] < '1' || s[0] > '9') {
      return l == 1 && s[0] == '0';
    }
  }
  constexpr size_t max_digits = std::numeric_limits<int64_t>::digits10 + 1;
  if (l == 0 || l > max_digits) {
    return false;
  }
  if (l == max_digits) {
    uint64_t val = s[0] - '0';
    for (size_t j = 1; j < l; j++) {
      if (s[j] > '9' || s[j] < '0') {
        return false;
      }
      val = val * 10 + s[j] - '0';
    }
    return val <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + has_minus;
  }

  for (size_t j = 1; j < l; j++) {
    if (s[j] > '9' || s[j] < '0') {
      return false;
    }
  }
  return true;
}


inline bool php_try_to_int(const char *s, size_t l, int64_t *val) __attribute__ ((always_inline));

bool php_try_to_int(const char *s, size_t l, int64_t *val) {
  int64_t mul = 1;
  if (l == 0) {
    return false;
  }
  if (s[0] == '-') {
    mul = -1;
    s++;
    l--;

    if (s[0] < '1' || s[0] > '9') {
      return false;
    }
  } else {
    if (s[0] < '1' || s[0] > '9') {
      *val = 0;
      return l == 1 && s[0] == '0';
    }
  }

  constexpr size_t max_digits = std::numeric_limits<int64_t>::digits10 + 1;
  if (l == 0 || l > max_digits) {
    return false;
  }
  if (l == max_digits) {
    *val = s[0] - '0';
    for (size_t j = 1; j < l; j++) {
      if (s[j] > '9' || s[j] < '0') {
        return false;
      }
      *val = *val * 10 + s[j] - '0';
    }

    if (*val > 0 || (*val == std::numeric_limits<int64_t>::min() && mul == -1)) {
      *val = *val * mul;
      return true;
    }
    return false;
  }

  *val = s[0] - '0';
  for (size_t j = 1; j < l; j++) {
    if (s[j] > '9' || s[j] < '0') {
      return false;
    }
    *val = *val * 10 + s[j] - '0';
  }

  *val *= mul;
  return true;
}

template<class T>
inline constexpr int three_way_comparison(const T &lhs, const T &rhs) {
  return lhs < rhs ? -1 :
         (rhs < lhs ?  1 : 0);
}
