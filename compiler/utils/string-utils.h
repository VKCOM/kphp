// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <climits>
#include <string>
#include <vector>

#include "common/algorithms/string-algorithms.h"
#include "common/wrappers/string_view.h"

inline std::string get_full_path(const std::string& file_name) {
  char name[PATH_MAX + 1];
  char* ptr = realpath(file_name.c_str(), name);

  if (ptr == nullptr) {
    return "";
  } else {
    return name;
  }
}

inline std::string as_dir(std::string path) noexcept {
  if (path.empty()) {
    return path;
  }
  auto full_path = get_full_path(path);
  if (!full_path.empty()) {
    path = std::move(full_path);
  }
  if (path.back() != '/') {
    path += "/";
  }
  if (path.front() != '/') {
    path.insert(0, "./");
  }
  return path;
}

static inline int is_alpha(int c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || (c == '_');
}

static inline int is_alphanum(int c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || (c == '_') || ('0' <= c && c <= '9');
}

static inline int is_digit(int c) {
  return '0' <= c && c <= '9';
}

static inline int conv_oct_digit(int c) {
  if ('0' <= c && c <= '7') {
    return c - '0';
  }
  return -1;
}

static inline int conv_hex_digit(int c) {
  if ('0' <= c && c <= '9') {
    return c - '0';
  }
  c |= 0x20;
  if ('a' <= c && c <= 'f') {
    return c - 'a' + 10;
  }
  return -1;
}

inline vk::string_view string_view_dup(vk::string_view s) {
  char* buf = new char[s.size()];
  memcpy(buf, s.begin(), s.size());
  return {buf, buf + s.size()};
}

inline std::vector<std::string> split(const std::string& s, char delimiter = ' ') {
  std::vector<std::string> res;

  int prev = 0;
  for (int i = 0; i <= (int)s.size(); i++) {
    if (s[i] == delimiter || s[i] == 0) {
      if (prev != i) {
        res.push_back(s.substr(prev, i - prev));
      }
      prev = i + 1;
    }
  }

  return res;
}

inline std::vector<vk::string_view> split_skipping_delimeters(vk::string_view str, vk::string_view delimiters = " ") {
  std::vector<vk::string_view> tokens;

  auto pos = str.find_first_not_of(delimiters, 0);
  auto next_delimiter = str.find_first_of(delimiters, pos);

  while (std::string::npos != pos || std::string::npos != next_delimiter) {
    tokens.push_back(str.substr(pos, next_delimiter - pos));

    pos = str.find_first_not_of(delimiters, next_delimiter);
    next_delimiter = str.find_first_of(delimiters, pos);
  }

  return tokens;
}

static inline std::string replace_characters(std::string s, char from, char to) {
  std::replace(s.begin(), s.end(), from, to);
  return s;
}

static inline void replace_non_alphanum_inplace(std::string& s, char to = '_') {
  // std::not_fn
  std::replace_if(s.begin(), s.end(), [](auto c) { return !is_alphanum(c); }, to);
}

static inline std::string replace_non_alphanum(std::string s, char to = '_') {
  replace_non_alphanum_inplace(s, to);
  return s;
}

static inline std::string replace_backslashes(const std::string& s) {
  return replace_characters(s, '\\', '$');
}

static inline std::string replace_call_string_to_readable(const std::string& s_with_$) {
  std::string repl = vk::replace_all(s_with_$, "$$", "::");
  repl = vk::replace_all(repl, "$", "\\");
  return repl;
}

static inline void remove_extra_spaces(std::string& str) {
  std::replace_if(str.begin(), str.end(), isspace, ' ');
  str = static_cast<std::string>(vk::trim(str));
  auto new_end = std::unique(str.begin(), str.end(), [](char lhs, char rhs) { return (lhs == rhs) && lhs == ' '; });
  str.erase(new_end, str.end());
}

static inline bool is_string_self_static_parent(const std::string& s) {
  return s == "self" || s == "static" || s == "parent";
}

std::string transform_to_snake_case(vk::string_view origin) noexcept;
std::string transform_to_camel_case(vk::string_view origin) noexcept;
