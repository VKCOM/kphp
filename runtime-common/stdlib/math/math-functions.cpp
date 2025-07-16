// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/math/math-functions.h"

#include <cctype>
#include <cstdint>
#include <cstring>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/string/string-context.h"
#include "runtime-common/stdlib/string/string-functions.h"

string f$dechex(int64_t number) noexcept {
  auto v = static_cast<uint64_t>(number);

  char s[17];
  int i = 16;

  const auto& lhex_digits{StringLibConstants::get().lhex_digits};
  do {
    s[--i] = lhex_digits[v & 15];
    v >>= 4;
  } while (v > 0);

  return {s + i, static_cast<string::size_type>(16 - i)};
}

int64_t f$hexdec(const string& number) noexcept {
  uint64_t v = 0;
  bool bad_str_param = number.empty();
  bool overflow = false;
  for (string::size_type i = 0; i < number.size(); i++) {
    const uint8_t d = hex_to_int(number[i]);
    if (unlikely(d == 16)) {
      bad_str_param = true;
    } else {
      v = math_functions_impl_::mult_and_add<16>(v, d, overflow);
    }
  }

  if (unlikely(bad_str_param)) {
    php_warning("Wrong parameter \"%s\" in function hexdec", number.c_str());
  }
  if (unlikely(overflow)) {
    php_warning("Integer overflow on converting '%s' in function hexdec, "
                "the result will be different from PHP",
                number.c_str());
  }
  return v;
}

string f$base_convert(const string& number, int64_t frombase, int64_t tobase) noexcept {
  if (frombase < 2 || frombase > 36) {
    php_warning("Wrong parameter frombase (%" PRIi64 ") in function base_convert", frombase);
    return number;
  }
  if (tobase < 2 || tobase > 36) {
    php_warning("Wrong parameter tobase (%" PRIi64 ") in function base_convert", tobase);
    return number;
  }

  string::size_type len = number.size();
  string::size_type f = 0;
  string result;
  if (number[0] == '-' || number[0] == '+') {
    f++;
    len--;
    if (number[0] == '-') {
      result.push_back('-');
    }
  }
  if (len == 0) {
    php_warning("Wrong parameter number (%s) in function base_convert", number.c_str());
    return number;
  }

  const char* digits = "0123456789abcdefghijklmnopqrstuvwxyz";

  string n(len, false);
  for (string::size_type i = 0; i < len; i++) {
    const char* s = static_cast<const char*>(std::memchr(digits, std::tolower(number[i + f]), 36));
    if (s == nullptr || s - digits >= frombase) {
      php_warning("Wrong character '%c' at position %u in parameter number (%s) in function base_convert", number[i + f], i + f, number.c_str());
      return number;
    }
    n[i] = static_cast<char>(s - digits);
  }

  int64_t um = 0;
  string::size_type st = 0;
  while (st < len) {
    um = 0;
    for (string::size_type i = st; i < len; i++) {
      um = um * frombase + n[i];
      n[i] = static_cast<char>(um / tobase);
      um %= tobase;
    }
    while (st < len && n[st] == 0) {
      st++;
    }
    result.push_back(digits[um]);
  }

  string::size_type i = f;
  int64_t j = int64_t{result.size()} - 1;
  while (i < j) {
    std::swap(result[i++], result[static_cast<string::size_type>(j--)]);
  }

  return result;
}
