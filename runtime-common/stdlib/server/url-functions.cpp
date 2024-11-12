// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/server/url-functions.h"

#include <cstdint>
#include <string_view>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/string/string-context.h"
#include "runtime-common/stdlib/string/string-functions.h"

namespace {

constexpr std::string_view good_url_symbols_mask = "00000000000000000000000000000000"
                                                   "00000000000001101111111111000000"
                                                   "01111111111111111111111111100001"
                                                   "01111111111111111111111111100000"
                                                   "00000000000000000000000000000000"
                                                   "00000000000000000000000000000000"
                                                   "00000000000000000000000000000000"
                                                   "00000000000000000000000000000000"; //[0-9a-zA-Z-_.]

} // namespace

string f$rawurlencode(const string &s) noexcept {
  auto &runtime_ctx{RuntimeContext::get()};
  runtime_ctx.static_SB.clean().reserve(3 * s.size());

  const auto &string_lib_constants{StringLibConstants::get()};
  for (string::size_type i = 0; i < s.size(); ++i) {
    if (good_url_symbols_mask[static_cast<unsigned char>(s[i])] == '1') {
      runtime_ctx.static_SB.append_char(s[i]);
    } else {
      runtime_ctx.static_SB.append_char('%');
      runtime_ctx.static_SB.append_char(string_lib_constants.uhex_digits[(s[i] >> 4) & 0x0F]);
      runtime_ctx.static_SB.append_char(string_lib_constants.uhex_digits[s[i] & 0x0F]);
    }
  }
  return runtime_ctx.static_SB.str();
}

string f$rawurldecode(const string &s) noexcept {
  auto &runtime_ctx{RuntimeContext::get()};
  runtime_ctx.static_SB.clean().reserve(s.size());

  for (string::size_type i = 0; i < s.size(); ++i) {
    if (s[i] == '%') {
      if (const uint32_t num_high{hex_to_int(s[i + 1])}; num_high < 16) {
        if (const uint32_t num_low{hex_to_int(s[i + 2])}; num_low < 16) {
          runtime_ctx.static_SB.append_char(static_cast<char>((num_high << 4) + num_low));
          i += 2;
          continue;
        }
      }
    }
    runtime_ctx.static_SB.append_char(s[i]);
  }
  return runtime_ctx.static_SB.str();
}

string f$urlencode(const string &s) noexcept {
  auto &runtime_ctx{RuntimeContext::get()};
  runtime_ctx.static_SB.clean().reserve(3 * s.size());

  const auto &string_lib_constants{StringLibConstants::get()};
  for (string::size_type i = 0; i < s.size(); ++i) {
    if (good_url_symbols_mask[static_cast<unsigned char>(s[i])] == '1') {
      runtime_ctx.static_SB.append_char(s[i]);
    } else if (s[i] == ' ') {
      runtime_ctx.static_SB.append_char('+');
    } else {
      runtime_ctx.static_SB.append_char('%');
      runtime_ctx.static_SB.append_char(string_lib_constants.uhex_digits[(s[i] >> 4) & 0x0F]);
      runtime_ctx.static_SB.append_char(string_lib_constants.uhex_digits[s[i] & 0x0F]);
    }
  }
  return runtime_ctx.static_SB.str();
}

string f$urldecode(const string &s) noexcept {
  auto &runtime_ctx{RuntimeContext::get()};
  runtime_ctx.static_SB.clean().reserve(s.size());

  for (string::size_type i = 0; i < s.size(); ++i) {
    if (s[i] == '%') {
      if (const uint32_t num_high{hex_to_int(s[i + 1])}; num_high < 16) {
        if (const uint32_t num_low{hex_to_int(s[i + 2])}; num_low < 16) {
          runtime_ctx.static_SB.append_char(static_cast<char>((num_high << 4) + num_low));
          i += 2;
          continue;
        }
      }
    } else if (s[i] == '+') {
      runtime_ctx.static_SB.append_char(' ');
      continue;
    }
    runtime_ctx.static_SB.append_char(s[i]);
  }
  return runtime_ctx.static_SB.str();
}
