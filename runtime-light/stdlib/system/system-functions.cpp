// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstddef>
#include <iconv.h>
#include <memory>

#include "common/containers/final_action.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/diagnostics.h"
#include "runtime-light/stdlib/system/system-functions.h"

Optional<string> f$iconv(const string& input_encoding, const string& output_encoding, const string& input_str) noexcept {
  iconv_t cd{};
  if (k2::iconv_open(std::addressof(cd), output_encoding.c_str(), input_encoding.c_str()) != k2::errno_ok) [[unlikely]] {
    kphp::log::warning("unsupported iconv from '{}' to '{}'", input_encoding.c_str(), output_encoding.c_str());
    return false;
  }

  vk::final_action finalizer{[&cd]() { k2::iconv_close(cd); }};

  for (int mul = 4; mul <= 16; mul *= 4) {
    string output_str{mul * input_str.size(), false};

    size_t input_len{input_str.size()};
    size_t output_len{output_str.size()};
    char* input_buf{const_cast<char*>(input_str.c_str())};
    char* output_buf{output_str.buffer()};
    size_t res{};
    if (k2::iconv(std::addressof(res), cd, std::addressof(input_buf), std::addressof(input_len), std::addressof(output_buf), std::addressof(output_len)) ==
        k2::errno_ok) {
      output_str.shrink(static_cast<string::size_type>(output_buf - output_str.c_str()));
      return output_str;
    }

    k2::iconv(std::addressof(res), cd, nullptr, nullptr, nullptr, nullptr);
  }

  return false;
}
