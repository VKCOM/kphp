// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace kphp::confdata {

enum class section_kind : uint8_t { simple_key, one_dot_wildcard, two_dots_wildcard, predefined_wildcard };

/**
 * @brief Classifies the syntactic form of a wildcard section.
 *
 * Only trailing-dot forms with exactly one or two dots are implicit sections. All other forms are
 * predefined-wildcard candidates and still require validation and presence in the configured index.
 */
inline auto classify_wildcard_form(std::string_view wildcard) noexcept -> section_kind {
  size_t dots{};
  if (!wildcard.empty() && wildcard.back() == '.') {
    for (const char c : wildcard) {
      dots += static_cast<size_t>(c == '.');
      if (dots > 2) {
        break;
      }
    }
  }
  switch (dots) {
  case 1:
    return section_kind::one_dot_wildcard;
  case 2:
    return section_kind::two_dots_wildcard;
  default:
    return section_kind::predefined_wildcard;
  }
}

} // namespace kphp::confdata
