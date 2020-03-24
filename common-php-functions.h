#pragma once

#include <climits>
#include <cstring>

constexpr int STRLEN_WARNING_FLAG = 1 << 30;
constexpr int STRLEN_OBJECT = -3;
constexpr int STRLEN_ERROR = -2;
constexpr int STRLEN_DYNAMIC = -1;
constexpr int STRLEN_UNKNOWN = STRLEN_ERROR;
constexpr int STRLEN_EMPTY = 0;
constexpr int STRLEN_BOOL = 1;
constexpr int STRLEN_BOOL_ = STRLEN_BOOL;
constexpr int STRLEN_INT = 11;
constexpr int STRLEN_FLOAT = 21;
constexpr int STRLEN_ARRAY = 5;
constexpr int STRLEN_ARRAY_ = STRLEN_ARRAY | STRLEN_WARNING_FLAG;
constexpr int STRLEN_STRING = STRLEN_DYNAMIC;
constexpr int STRLEN_VAR = STRLEN_DYNAMIC;
constexpr int STRLEN_UINT = STRLEN_OBJECT;
constexpr int STRLEN_LONG = STRLEN_OBJECT;
constexpr int STRLEN_ULONG = STRLEN_OBJECT;
constexpr int STRLEN_CLASS = STRLEN_ERROR;
constexpr int STRLEN_VOID = STRLEN_ERROR;
constexpr int STRLEN_ANY = STRLEN_ERROR;
constexpr int STRLEN_FUTURE = STRLEN_ERROR;
constexpr int STRLEN_FUTURE_QUEUE = STRLEN_ERROR;
constexpr int STRLEN_CREATE_ANY = STRLEN_ERROR;

class ExtraRefCnt {
public:
  enum extra_ref_cnt_value {
    // используется в качестве референс каунтера для глобальных констант,
    // которые обычно используются в качетсве дефолтных значений для различных переменных,
    // данные находятся либо на куче, либо в data секции и доступны только на чтение
    for_global_const = 0x7ffffff0,

    // используется в качестве референс каунтера для переменных хранящихся в инстанс кеше (instance_cache),
    // данные хранятся в шерной памяти между процессами,
    // подразумевает рекурсивное удаление для массивов
    for_instance_cache,

    // используется в качестве референс каунтера для переменных хранящихся в конфдата хранилище,
    // данные хранятся в шерной памяти между процессами,
    // подразумевает нерекурсивное и рекурсивное удаление для массивов (в зависимости от контекста)
    for_confdata
  };

  // Обертка в класс позволяет не засорять глобальный неймспейс
  ExtraRefCnt(extra_ref_cnt_value value) noexcept:
    value_(value) {}

  operator int() const noexcept {
    return static_cast<int>(value_);
  }

private:
  extra_ref_cnt_value value_;
};

inline int string_hash(const char *p, int l) __attribute__ ((always_inline));

int string_hash(const char *p, int l) {
  static const unsigned int HASH_MUL_ = 1915239017;
  unsigned int hash = 2147483648u;

  int prev = (l & 3);
  for (int i = 0; i < prev; i++) {
    hash = hash * HASH_MUL_ + p[i];
  }

  const unsigned int *p_uint = (unsigned int *)(p + prev);
  l >>= 2;
  while (l-- > 0) {
    hash = hash * HASH_MUL_ + *p_uint++;
  }
  return (int)hash;
}

inline bool php_is_int(const char *s, int l) __attribute__ ((always_inline));

bool php_is_int(const char *s, int l) {
  if ((s[0] - '-') * (s[0] - '+') == 0) { // no need to check l > 0
    s++;
    l--;

    if ((unsigned int)(s[0] - '1') > 8) {
      return false;
    }
  } else {
    if ((unsigned int)(s[0] - '1') > 8) {
      return l == 1 && s[0] == '0';
    }
  }

  if ((unsigned int)(l - 1) > 9) {
    return false;
  }
  if (l == 10) {
    int val = 0;
    for (int j = 1; j < l; j++) {
      if (s[j] > '9' || s[j] < '0') {
        return false;
      }
      val = val * 10 + s[j] - '0';
    }

    if (s[0] != '2') {
      return s[0] == '1';
    }
    return val < 147483648;
  }

  for (int j = 1; j < l; j++) {
    if (s[j] > '9' || s[j] < '0') {
      return false;
    }
  }
  return true;
}

inline bool php_try_to_int(const char *s, int l, int *val) __attribute__ ((always_inline));

bool php_try_to_int(const char *s, int l, int *val) {
  int mul;
  if (s[0] == '-') { // no need to check l > 0
    mul = -1;
    s++;
    l--;

    if ((unsigned int)(s[0] - '1') > 8) {
      return false;
    }
  } else {
    if ((unsigned int)(s[0] - '1') > 8) {
      *val = 0;
      return l == 1 && s[0] == '0';
    }
    mul = 1;
  }

  if ((unsigned int)(l - 1) > 9) {
    return false;
  }
  if (l == 10) {
    if (s[0] > '2') {
      return false;
    }

    *val = s[0] - '0';
    for (int j = 1; j < l; j++) {
      if (s[j] > '9' || s[j] < '0') {
        return false;
      }
      *val = *val * 10 + s[j] - '0';
    }

    if (*val > 0 || (*val == -2147483648 && mul == -1)) {
      *val = *val * mul;
      return true;
    }
    return false;
  }

  *val = s[0] - '0';
  for (int j = 1; j < l; j++) {
    if (s[j] > '9' || s[j] < '0') {
      return false;
    }
    *val = *val * 10 + s[j] - '0';
  }

  *val *= mul;
  return true;
}

//returns len of raw string representation or -1 on error
inline int string_raw_len(int src_len) {
  if (src_len < 0 || src_len >= (1 << 30) - 13) {
    return -1;
  }

  return src_len + 13;
}

//returns len of raw string representation and writes it to dest or returns -1 on error
inline int string_raw(char *dest, int dest_len, const char *src, int src_len) {
  int raw_len = string_raw_len(src_len);
  if (raw_len == -1 || raw_len > dest_len) {
    return -1;
  }
  int *dest_int = reinterpret_cast <int *> (dest);
  dest_int[0] = src_len;
  dest_int[1] = src_len;
  dest_int[2] = ExtraRefCnt::for_global_const;
  memcpy(dest + 3 * sizeof(int), src, src_len);
  dest[3 * sizeof(int) + src_len] = '\0';

  return raw_len;
}

template<class T>
inline constexpr int three_way_comparison(const T &lhs, const T &rhs) {
  return lhs < rhs ? -1 :
         (lhs > rhs ?  1 : 0);
}
