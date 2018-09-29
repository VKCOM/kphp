#pragma once

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

class string_ref {
private:
  const char *s, *t;

public:
  string_ref();
  string_ref(const char *s, const char *t);

  int length() const;

  const char *begin() const;
  const char *end() const;

  string str() const;
  const char *c_str() const;

  bool starts_with(const char *rhs) const;

  inline operator string() const {
    return string(s, t);
  }
};

inline string_ref::string_ref() :
  s(nullptr),
  t(nullptr) {}

inline string_ref::string_ref(const char *s, const char *t) :
  s(s),
  t(t) {}

inline int string_ref::length() const {
  return (int)(t - s);
}

inline const char *string_ref::begin() const {
  return s;
}

inline const char *string_ref::end() const {
  return t;
}

inline bool operator==(const string_ref &lhs, const char *rhs) {
  int len = (int)strlen(rhs);
  return len == lhs.length() && strncmp(rhs, lhs.begin(), len) == 0;
}

inline string_ref string_ref_dup(const string &s) {
  char *buf = new char[s.length()];
  memcpy(buf, &s[0], s.size());
  return string_ref(buf, buf + s.length());
}

inline string string_ref::str() const {
  return string(begin(), end());
}

inline const char *string_ref::c_str() const {
  return str().c_str();
}

inline bool string_ref::starts_with(const char *str) const {
  int len = (int)strlen(str);
  return len <= length() && strncmp(str, begin(), len) == 0;
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

template<class T>
void clear(T &val) {
  val.clear();
}

template<class ArrayType, class IndexType>
IndexType dsu_get(ArrayType *arr, IndexType i) {
  if ((*arr)[i] == i) {
    return i;
  }
  return (*arr)[i] = dsu_get(arr, (*arr)[i]);
}

template<class ArrayType, class IndexType>
void dsu_uni(ArrayType *arr, IndexType i, IndexType j) {
  i = dsu_get(arr, i);
  j = dsu_get(arr, j);
  if (!(i == j)) {
    if (rand() & 1) {
      (*arr)[i] = j;
    } else {
      (*arr)[j] = i;
    }
  }
}

template<class T>
void my_unique(T *v) {
  sort(v->begin(), v->end());
  v->erase(std::unique(v->begin(), v->end()), v->end());
}
