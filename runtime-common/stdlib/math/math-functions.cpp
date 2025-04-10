// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/math/math-functions.h"

#include <cstdint>

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
