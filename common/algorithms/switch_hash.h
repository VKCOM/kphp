// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

namespace vk {

inline bool validate_case_hash8_string(const char *s, int64_t s_len) {
  constexpr int short_string_max_len = 8;
  if (s_len > short_string_max_len || s_len == 0) {
    return false;
  }
  // not using std::any_of here to avoid extra dependencies in runtime string decl header
  for (int i = 0; i < s_len; i++) {
    if (s[i] == 0) {
      return false;
    }
  }
  return true;
}

// case_hash8 calculates a short string hash without collisions
// can be used by both compiler and runtime;
//
// for compiler usage, validate_case_hash8_string should be used
// to check that all switch cases produce valid hash
//
// note that this function returns 0 for cases that should be handled
// by the "default" clause, this is why empty strings are not a
// valid candidate for hashing by this function
template<int MaxLen = 8>
uint64_t case_hash8(const char *s, int64_t s_len) {
  static_assert(MaxLen >= 2 && MaxLen <= 8);
  uint64_t h = 0;
  if (s_len <= MaxLen) {
    // terminating null byte can screw the hash sum;
    // cases never have a null byte, so we throw away
    // this string without calculating its hash
    if (s[s_len-1] == 0) {
      return 0;
    }
    // since compiler sees a condition over a constant size MaxLen above,
    // it compiles this memcpy as a series of mov instructions to copy
    // the data without an actual call or loops
    memcpy(&h, s, s_len);
  }
  return h;
}

// a special case for cases with len 1
template<>
inline uint64_t case_hash8<1>(const char *s, int64_t s_len) {
  return s_len == 1 ? static_cast<unsigned char>(s[0]) : 0;
}

} // namespace vk
