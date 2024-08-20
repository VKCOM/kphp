#pragma once

#include "runtime-core/runtime-core.h"
#include <type_traits>

void print(const char *s, size_t s_len);

void print(const char *s);

void print(const string &s);

void print(const string_buffer &sb);

inline void f$echo(const string &s) {
  print(s);
}

inline int64_t f$print(const string &s) {
  print(s);
  return 1;
}

inline int64_t f$strlen(const string &s) {
  return s.size();
}

void f$debug_print_string(const string &s);

string f$increment_byte(const string &s);

Optional<int64_t> f$byte_to_int(const string &s);

Optional<string> f$int_to_byte(int64_t v);

// str_concat_arg generalizes both tmp_string and string arguments;
// it can be constructed from both of them, so concat functions can operate
// on both tmp_string and string types
// there is a special (string, string) overloading for concat2 to
// allow the empty string result optimization to kick in
struct str_concat_arg {
  const char *data;
  string::size_type size;

  str_concat_arg(const string &s)
    : data{s.c_str()}
    , size{s.size()} {}
  str_concat_arg(tmp_string s)
    : data{s.data}
    , size{s.size} {}

  tmp_string as_tmp_string() const noexcept {
    return {data, size};
  }
};

// str_concat functions implement efficient string-typed `.` (concatenation) operator implementation;
// apart from being machine-code size efficient (a function call is more compact), they're also
// usually faster as runtime is compiled with -O3 which is almost never the case for translated C++ code
// (it's either -O2 or -Os most of the time)
//
// we choose to have 4 functions (up to 5 arguments) because of the frequency distribution:
//   37619: 2 args
//   20616: 3 args
//    4534: 5 args
//    3791: 4 args
//     935: 7 args
//     565: 6 args
//     350: 9 args
// Both 6 and 7 argument combination already look infrequent enough to not bother
string str_concat(const string &s1, const string &s2);
string str_concat(str_concat_arg s1, str_concat_arg s2);
string str_concat(str_concat_arg s1, str_concat_arg s2, str_concat_arg s3);
string str_concat(str_concat_arg s1, str_concat_arg s2, str_concat_arg s3, str_concat_arg s4);
string str_concat(str_concat_arg s1, str_concat_arg s2, str_concat_arg s3, str_concat_arg s4, str_concat_arg s5);
