// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <array>

#include "runtime/kphp_core.h"

#include "common/wrappers/fmt_format.h"

// Don't move this destructor to the headers, it spoils addr2line traces
string::~string() noexcept {
  destroy();
}

string::string(double f) {
  constexpr uint32_t MAX_LEN = 4096;
  std::array<char, MAX_LEN + 2> result = {};

  char *begin = result.data() + 2;
  if (std::isnan(f)) {
    // to prevent printing `-NAN`
    f = std::abs(f);
  }

  // fmt::format uses Grisu algorithm for double to string conversions,
  // which is 3-5 times faster than snprintf one.
  // check benchmarks in https://github.com/fmtlib/fmt
  int len = fmt::format_to_n(begin, MAX_LEN, "{:.14G}", f).size;

  if (static_cast<uint32_t>(len) < MAX_LEN) {
    if (static_cast<uint32_t>(begin[len - 1] - '5') < 5 && begin[len - 2] == '0' && begin[len - 3] == '-') {
      --len;
      begin[len - 1] = begin[len];
    }
    if (begin[1] == 'E') {
      result[0] = begin[0];
      result[1] = '.';
      result[2] = '0';
      begin = result.data();
      len += 2;
    } else if (begin[0] == '-' && begin[2] == 'E') {
      result[0] = begin[0];
      result[1] = begin[1];
      result[2] = '.';
      result[3] = '0';
      begin = result.data();
      len += 2;
    }
    php_assert (len <= STRLEN_FLOAT);
    p = create(begin, begin + len);
  } else {
    php_warning("Maximum length of float (%d) exceeded", MAX_LEN);
    p = string_cache::empty_string().ref_data();
  }
}
