// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <iconv.h>

#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/system/system-functions.h"

Optional<string> f$iconv(const string &input_encoding, const string &output_encoding, const string &input_str) noexcept {
  iconv_t cd;
  if (k2::iconv_open(&cd, output_encoding.c_str(), input_encoding.c_str()) != k2::errno_ok) {
    php_warning("unsupported iconv from \"%s\" to \"%s\"", input_encoding.c_str(), output_encoding.c_str());
    return false;
  }

  for (int mul = 4; mul <= 16; mul *= 4) {
    string output_str{mul * input_str.size(), false};

    size_t input_len{input_str.size()};
    size_t output_len{output_str.size()};
    char *input_buf{const_cast<char *>(input_str.c_str())};
    char *output_buf{output_str.buffer()};
    size_t res{};
    if (k2::iconv(&res, cd, &input_buf, &input_len, &output_buf, &output_len) == k2::errno_ok) {
      output_str.shrink(static_cast<string::size_type>(output_buf - output_str.c_str()));
      k2::iconv_close(cd);
      return output_str;
    }

    k2::iconv(&res, cd, nullptr, nullptr, nullptr, nullptr);
  }

  k2::iconv_close(cd);

  return false;
}
