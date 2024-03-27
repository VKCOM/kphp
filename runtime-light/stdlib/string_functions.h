#pragma once

#include "runtime-light/core/kphp_core.h"
#include <type_traits>

void print(const char *s, size_t s_len);

void print(const char *s);

void print(const string &s);

void print(const string_buffer &sb);

inline void f$echo(const string& s) {
  print(s);
}

inline int64_t f$print(const string& s) {
  print(s);
  return 1;
}