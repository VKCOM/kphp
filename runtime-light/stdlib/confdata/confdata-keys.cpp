// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/confdata/confdata-keys.h"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <string_view>

#include "common/php-functions.h"

namespace {

/**
 * @brief Normalizes a key remainder like a PHP array key: numeric strings become `int64_t`.
 */
auto normalize_remainder(std::string_view remainder) noexcept -> kphp::confdata::key_views::remainder_type {
  int64_t remainder_as_int{0};
  if (!remainder.empty() && php_try_to_int(remainder.data(), remainder.size(), std::addressof(remainder_as_int))) {
    return {remainder_as_int};
  }
  return {remainder};
}

} // namespace

namespace kphp::confdata {

auto split_key(std::string_view key) noexcept -> std::expected<key_views, split_error> {
  if (key.size() > MAX_KEY_LENGTH) [[unlikely]] {
    return std::unexpected{split_error::key_too_long};
  }

  const auto first_dot{key.find('.')};
  if (first_dot == std::string_view::npos) {
    return key_views{section_kind::simple_key, key, key, key_views::remainder_type{}};
  }
  const auto second_dot{key.find('.', first_dot + 1)};
  if (second_dot == std::string_view::npos) {
    return key_views{section_kind::one_dot_wildcard, key, key.substr(0, first_dot + 1), normalize_remainder(key.substr(first_dot + 1))};
  }
  return key_views{section_kind::two_dots_wildcard, key, key.substr(0, second_dot + 1), normalize_remainder(key.substr(second_dot + 1))};
}

auto split_key(std::string_view key, const predefined_wildcards& wildcards) noexcept -> std::expected<key_views, split_error> {
  // if the key has a predefined wildcard prefix, use the shortest matching one as the section
  if (const auto opt_wildcard{wildcards.shortest_matching_wildcard(key)}; opt_wildcard.has_value()) {
    return split_key_with_predefined_wildcard(key, opt_wildcard->size());
  }
  return split_key(key);
}

auto split_key_with_predefined_wildcard(std::string_view key, size_t wildcard_len) noexcept -> std::expected<key_views, split_error> {
  if (key.size() > MAX_KEY_LENGTH) [[unlikely]] {
    return std::unexpected{split_error::key_too_long};
  }
  if (wildcard_len == 0 || wildcard_len > key.size()) [[unlikely]] {
    return std::unexpected{split_error::invalid_predefined_wildcard_length};
  }
  return key_views{section_kind::predefined_wildcard, key, key.substr(0, wildcard_len), normalize_remainder(key.substr(wildcard_len))};
}

} // namespace kphp::confdata
