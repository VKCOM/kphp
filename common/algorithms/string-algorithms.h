// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "common/functional/identity.h"
#include "common/smart_iterators/transform_iterator.h"
#include "common/wrappers/string_view.h"

namespace vk {

template<typename Container, typename Mapper = vk::identity,
  typename = decltype(std::declval<Container>().begin()),
  typename = decltype(std::declval<Container>().end())>
std::string join(const Container &v, const std::string &delim, Mapper mapper = {}) {
  if (v.empty()) {
    return "";
  }
  std::string res;
  res.reserve(std::distance(std::begin(v), std::end(v)) * (delim.size() + 10));
  auto it = vk::make_transform_iterator(mapper, std::begin(v));
  auto end = vk::make_transform_iterator(mapper, std::end(v));
  res.append(*it);
  while (++it != end) {
    res.append(delim).append(*it);
  }
  return res;
}

inline bool ends_with(const std::string &s, const std::string &suffix) {
  return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

inline std::string to_lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), ::tolower);
  return s;
}

inline std::string to_upper(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), ::toupper);
  return s;
}

inline vk::string_view ltrim(vk::string_view s) {
  const auto *first_not_space = std::find_if(s.begin(), s.end(), [](uint8_t ch) { return !std::isspace(ch); });
  return {first_not_space, s.end()};
}

inline vk::string_view rtrim(vk::string_view s) {
  const auto *last_not_space = std::find_if(s.rbegin(), s.rend(), [](uint8_t ch) { return !std::isspace(ch); }).base();
  return {s.begin(), last_not_space};
}

inline vk::string_view trim(vk::string_view s) {
  return ltrim(rtrim(s));
}

inline std::string replace_all(const std::string &haystack, const std::string &find, const std::string &replace) {
  std::string res = haystack;
  size_t start_pos = 0;

  while(std::string::npos != (start_pos = res.find(find, start_pos))) {
    res.replace(start_pos, find.length(), replace);
    start_pos += replace.length();
  }
  return res;
}

inline bool is_ascii_whitespace(char c) noexcept {
  return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

inline vk::string_view strip_ascii_whitespace(vk::string_view view) noexcept {
  const auto *not_space = std::find_if_not(view.begin(), view.end(), is_ascii_whitespace);
  view = vk::string_view{not_space, view.end()};
  not_space = std::find_if_not(view.rbegin(), view.rend(), is_ascii_whitespace).base();
  return vk::string_view{view.begin(), not_space};
}

inline std::pair<vk::string_view, vk::string_view> split_string_view(vk::string_view view, char delimiter) noexcept {
  size_t pos = view.find(delimiter);
  if (pos == vk::string_view::npos) {
    return {view, vk::string_view{}};
  }
  return {view.substr(0, pos), view.substr(pos + 1)};
}

} // namespace vk
