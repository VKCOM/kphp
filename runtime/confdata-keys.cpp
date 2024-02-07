// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/confdata-keys.h"

#include "common/algorithms/contains.h"

void ConfdataPredefinedWildcards::set_wildcards(std::unordered_set<vk::string_view> &&wildcards) noexcept {
  shortest_wildcard_ = vk::string_view::npos;
  wildcards_ = std::move(wildcards);
  for (auto it = wildcards_.begin(); it != wildcards_.end();) {
    if (get_wildcard_type(*it) != ConfdataFirstKeyType::predefined_wildcard) {
      it = wildcards_.erase(it);
      continue;
    }
    shortest_wildcard_ = std::min(shortest_wildcard_, it->size());
    ++it;
  }

  wildcards_by_prefix_.clear();
  for (const auto &wildcard : wildcards_) {
    auto &suffixes = wildcards_by_prefix_[wildcard.substr(0, shortest_wildcard_)];
    suffixes.emplace_back(wildcard.substr(shortest_wildcard_));
  }

  max_wildcards_for_element_ = 0;
  for (auto &wildcard_suffixes : wildcards_by_prefix_) {
    max_wildcards_for_element_ = std::max(max_wildcards_for_element_, wildcard_suffixes.second.size());
    std::sort(wildcard_suffixes.second.begin(), wildcard_suffixes.second.end());
  }
}

ConfdataFirstKeyType ConfdataPredefinedWildcards::detect_first_key_type(vk::string_view first_key) const noexcept {
  const auto type = get_wildcard_type(first_key);
  return type != ConfdataFirstKeyType::predefined_wildcard || has_wildcard(first_key)
         ? type
         : ConfdataFirstKeyType::simple_key;
}

ConfdataFirstKeyType ConfdataPredefinedWildcards::get_wildcard_type(vk::string_view wildcard) noexcept {
  size_t dots = 0;
  if (!wildcard.empty() && *wildcard.rbegin() == '.') {
    for (const auto *c = wildcard.begin(); c != wildcard.end() && dots <= 2; ++c) {
      dots += (*c == '.');
    }
  }
  switch (dots) {
    case 1:
      return ConfdataFirstKeyType::one_dot_wildcard;
    case 2:
      return ConfdataFirstKeyType::two_dots_wildcard;
    default:
      return ConfdataFirstKeyType::predefined_wildcard;
  }
}

ConfdataFirstKeyType ConfdataKeyMaker::update(const char *key, int16_t key_len, const ConfdataPredefinedWildcards &wildcards) noexcept {
  php_assert(key_len >= 0);
  const vk::string_view key_view{key, static_cast<size_t>(key_len)};
  const auto predefined_wildcards = wildcards.make_predefined_wildcard_len_range_by_key(key_view);
  // if key has a predefined prefix
  if (!predefined_wildcards.empty()) {
    const size_t wildcard_len = *predefined_wildcards.begin();
    php_assert(wildcard_len <= static_cast<size_t>(std::numeric_limits<int16_t>::max()));
    update_with_predefined_wildcard(key, key_len, static_cast<int16_t>(wildcard_len));
  } else {
    update(key, key_len);
  }
  return first_key_type_;
}

void ConfdataKeyMaker::reset_raw(const char *key, int16_t key_len) noexcept {
  php_assert(key_len >= 0);
  raw_key_ = key;
  raw_key_len_ = key_len;
  first_key_ = string{};
  second_key_ = mixed{};
}

void ConfdataKeyMaker::init_second_key(const char *second_key, int16_t second_key_key_len) noexcept {
  int64_t key_as_int = 0;
  php_assert(second_key_key_len >= 0);
  if (second_key_key_len && php_try_to_int(second_key, second_key_key_len, &key_as_int)) {
    second_key_ = key_as_int;
  } else {
    second_key_ = string::make_const_string_on_memory(second_key, static_cast<string::size_type>(second_key_key_len),
                                                      second_key_buffer_.data(), second_key_buffer_.size());
  }
}

ConfdataFirstKeyType ConfdataKeyMaker::update(const char *key, int16_t key_len) noexcept {
  reset_raw(key, key_len);

  const char *dot_one_end = std::find(key, key + key_len, '.');
  if (dot_one_end++ == key + key_len) {
    first_key_type_ = ConfdataFirstKeyType::simple_key;
    first_key_ = string::make_const_string_on_memory(key, key_len, first_key_buffer_.data(), first_key_buffer_.size());
    return first_key_type_;
  }
  const char *dot_two_end = std::find(dot_one_end, key + key_len, '.');
  const char *first_key_end = dot_one_end;
  if (dot_two_end++ == key + key_len) {
    first_key_type_ = ConfdataFirstKeyType::one_dot_wildcard;
  } else {
    first_key_type_ = ConfdataFirstKeyType::two_dots_wildcard;
    first_key_end = dot_two_end;
  }
  const auto first_key_len = static_cast<string::size_type>(first_key_end - key);
  first_key_ = string::make_const_string_on_memory(key, first_key_len, first_key_buffer_.data(), first_key_buffer_.size());
  init_second_key(first_key_end, static_cast<int16_t>(key_len - first_key_len));
  return first_key_type_;
}

void ConfdataKeyMaker::update_with_predefined_wildcard(const char *key, int16_t key_len, int16_t wildcard_len) noexcept {
  reset_raw(key, key_len);
  php_assert(wildcard_len > 0 && wildcard_len <= key_len);
  first_key_type_ = ConfdataFirstKeyType::predefined_wildcard;
  first_key_ = string::make_const_string_on_memory(key, static_cast<string::size_type>(wildcard_len),
                                                   first_key_buffer_.data(), first_key_buffer_.size());
  init_second_key(key + wildcard_len, static_cast<int16_t>(key_len - wildcard_len));
}

void ConfdataKeyMaker::forcibly_change_first_key_wildcard_dots_from_two_to_one() noexcept {
  php_assert(first_key_type_ == ConfdataFirstKeyType::two_dots_wildcard);
  const char *first_key_end = std::find(raw_key_, raw_key_ + raw_key_len_, '.') + 1;
  php_assert(first_key_end != raw_key_ + raw_key_len_ + 1);
  const auto first_key_len = static_cast<string::size_type>(first_key_end - raw_key_);
  const auto second_key_len = static_cast<string::size_type>(raw_key_len_ - first_key_len);
  first_key_ = string::make_const_string_on_memory(raw_key_, first_key_len, first_key_buffer_.data(), first_key_buffer_.size());
  second_key_ = string::make_const_string_on_memory(first_key_end, second_key_len, second_key_buffer_.data(), second_key_buffer_.size());
  first_key_type_ = ConfdataFirstKeyType::one_dot_wildcard;
}

bool ConfdataKeyBlacklist::is_key_forcibly_ignored_by_prefix(vk::string_view key) const noexcept {
  for (const vk::string_view &prefix : force_ignore_prefixes_) {
    if (key.starts_with(prefix)) {
      return true;
    }
  }
  return false;
}
