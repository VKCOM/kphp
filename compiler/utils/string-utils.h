#pragma once

#include "common/wrappers/string_view.h"

#include "compiler/common.h"

inline string get_full_path(const string &file_name) {
  char name[PATH_MAX + 1];
  char *ptr = realpath(file_name.c_str(), name);

  if (ptr == nullptr) {
    return "";
  } else {
    return name;
  }
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

inline vk::string_view string_view_dup(const vk::string_view &s) {
  char *buf = new char[s.size()];
  memcpy(buf, s.begin(), s.size());
  return vk::string_view(buf, buf + s.size());
}

inline string int_to_str(int x) {
  char tmp[50];
  sprintf(tmp, "%d", x);
  return tmp;
}

inline vector<string> split(const string &s, char delimiter = ' ') {
  vector<string> res;

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

inline std::vector<std::string> split_skipping_delimeters(const std::string &str, const std::string &delimiters = " ") {
  using std::string;

  std::vector<string> tokens;

  string::size_type pos = str.find_first_not_of(delimiters, 0);
  string::size_type next_delimiter = str.find_first_of(delimiters, pos);

  while (string::npos != pos || string::npos != next_delimiter) {
    tokens.push_back(str.substr(pos, next_delimiter - pos));

    pos = str.find_first_not_of(delimiters, next_delimiter);
    next_delimiter = str.find_first_of(delimiters, pos);
  }

  return tokens;
}

static inline void ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
    return !std::isspace(ch);
  }));
}

static inline void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
    return !std::isspace(ch);
  }).base(), s.end());
}

static inline void trim(std::string &s) {
  ltrim(s);
  rtrim(s);
}

static inline string replace_characters(string s, char from, char to) {
  std::replace(s.begin(), s.end(), from, to);
  return s;
}

static inline string replace_backslashes(const string &s) {
  return replace_characters(s, '\\', '$');
}
