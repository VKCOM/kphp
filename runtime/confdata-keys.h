// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <array>
#include <re2/re2.h>
#include <forward_list>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>

#include "common/mixin/not_copyable.h"
#include "common/wrappers/iterator_range.h"
#include "common/wrappers/string_view.h"

#include "runtime-core/runtime-core.h"

enum class ConfdataFirstKeyType {
  simple_key,
  one_dot_wildcard,
  two_dots_wildcard,
  predefined_wildcard
};

class ConfdataPredefinedWildcards : vk::not_copyable {
public:
  void set_wildcards(std::unordered_set<vk::string_view> &&wildcards) noexcept;

  auto make_predefined_wildcard_len_range_by_key(vk::string_view key) const noexcept {
    const vk::string_view *begin = nullptr;
    const vk::string_view *end = nullptr;
    vk::string_view key_tail;
    if (key.size() >= shortest_wildcard_) {
      const auto wildcards_it = wildcards_by_prefix_.find(key.substr(0, shortest_wildcard_));
      if (wildcards_it != wildcards_by_prefix_.end()) {
        key_tail = key.substr(shortest_wildcard_);
        begin = wildcards_it->second.data();
        end = begin + wildcards_it->second.size();
      }
    }
    auto r = vk::make_filter_iterator_range(
      [key_tail](vk::string_view wildcard_tail) {
        return key_tail.starts_with(wildcard_tail);
      }, begin, end);
    return vk::make_transform_iterator_range(
      [this](vk::string_view wildcard_tail) {
        return shortest_wildcard_ + wildcard_tail.size();
      }, r.begin(), r.end());
  }

  size_t get_max_wildcards_for_element() const noexcept {
    return max_wildcards_for_element_;
  }

  bool has_wildcard(vk::string_view wildcard) const noexcept {
    return wildcards_.find(wildcard) != wildcards_.end();
  }

  ConfdataFirstKeyType detect_first_key_type(vk::string_view first_key) const noexcept;

  bool is_most_common_predefined_wildcard(vk::string_view predefined_wildcard) const noexcept {
    const auto range = make_predefined_wildcard_len_range_by_key(predefined_wildcard);
    assert(range.begin() != range.end());
    return std::next(range.begin()) == range.end();
  }

  bool has_wildcard_for_key(vk::string_view first_key) const noexcept {
    return !make_predefined_wildcard_len_range_by_key(first_key).empty();
  }

private:
  static ConfdataFirstKeyType get_wildcard_type(vk::string_view wildcard) noexcept;

  size_t max_wildcards_for_element_{0};
  size_t shortest_wildcard_{vk::string_view::npos};
  std::unordered_map<vk::string_view, std::vector<vk::string_view>> wildcards_by_prefix_;
  std::unordered_set<vk::string_view> wildcards_;
};

class ConfdataKeyMaker : vk::not_copyable {
public:
  ConfdataKeyMaker() = default;

  ConfdataFirstKeyType update(const char *key, int16_t key_len) noexcept;
  ConfdataFirstKeyType update(const char *key, int16_t key_len, const ConfdataPredefinedWildcards &wildcards) noexcept;

  void update_with_predefined_wildcard(const char *key, int16_t key_len, int16_t wildcard_len) noexcept;

  void forcibly_change_first_key_wildcard_dots_from_two_to_one() noexcept;

  ConfdataFirstKeyType get_first_key_type() const noexcept { return first_key_type_; }

  const string &get_first_key() const noexcept { return first_key_; }
  const mixed &get_second_key() const noexcept { return second_key_; }

  string make_first_key_copy() const noexcept { return first_key_.copy_and_make_not_shared(); }
  mixed make_second_key_copy() const noexcept { return second_key_.is_string() ? mixed{second_key_.as_string().copy_and_make_not_shared()} : second_key_; }

private:
  void reset_raw(const char *key, int16_t key_len) noexcept;
  void init_second_key(const char *second_key, int16_t second_key_key_len) noexcept;

  const char *raw_key_{nullptr};
  short raw_key_len_{0};

  ConfdataFirstKeyType first_key_type_{ConfdataFirstKeyType::simple_key};
  string first_key_;
  mixed second_key_;

  std::array<char, string::inner_sizeof() + std::numeric_limits<int16_t>::max() + 1> first_key_buffer_;
  std::array<char, string::inner_sizeof() + std::numeric_limits<int16_t>::max() + 1> second_key_buffer_;
};

class ConfdataKeyBlacklist : vk::not_copyable {
  bool is_key_forcibly_ignored_by_prefix(vk::string_view key) const noexcept;

public:
  void set_blacklist(std::unique_ptr<re2::RE2> &&blacklist_pattern, std::forward_list<vk::string_view> &&force_ignore_prefixes) noexcept {
    blacklist_pattern_ = std::move(blacklist_pattern);
    force_ignore_prefixes_ = std::move(force_ignore_prefixes);
  }

  bool is_blacklisted(vk::string_view key) const noexcept {
    // from PHP class KphpConfiguration, e.g. for langs
    if (blacklist_pattern_ &&
           re2::RE2::FullMatch(
             re2::StringPiece(key.data(), static_cast<uint32_t>(key.size())),
             *blacklist_pattern_)) {
      return true;
    }
    // emergency startup option to disable keys by prefix, e.g. 'highload.vid'
    if (unlikely(!force_ignore_prefixes_.empty()) &&
        is_key_forcibly_ignored_by_prefix(key)) {
      return true;
    }
    return false;
  }

private:
  std::unique_ptr<re2::RE2> blacklist_pattern_;
  std::forward_list<vk::string_view> force_ignore_prefixes_;
};
