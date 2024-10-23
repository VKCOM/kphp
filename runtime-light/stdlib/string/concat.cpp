// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/string/concat.h"

#include "runtime-common/runtime-core/runtime-core.h"

string str_concat(const string &s1, const string &s2) {
  // for 2 argument concatenation it's not so uncommon to have at least one empty string argument;
  // it happens in cases like `$prefix . $s` where $prefix could be empty depending on some condition
  // real-world applications analysis shows that ~17.6% of all two arguments concatenations have
  // at least one empty string argument
  //
  // checking both lengths for 0 is almost free, but when we step into those 17.6%, we get almost x10
  // faster concatenation and no heap allocations
  //
  // this idea is borrowed from the Go runtime
  if (s1.empty()) {
    return s2;
  }
  if (s2.empty()) {
    return s1;
  }

  const auto new_size{s1.size() + s2.size()};
  return string{new_size, true}.append_unsafe(s1).append_unsafe(s2).finish_append();
}

string str_concat(str_concat_arg s1, str_concat_arg s2) {
  const auto new_size{s1.size + s2.size};
  return string{new_size, true}.append_unsafe(s1.as_tmp_string()).append_unsafe(s2.as_tmp_string()).finish_append();
}

string str_concat(str_concat_arg s1, str_concat_arg s2, str_concat_arg s3) {
  const auto new_size{s1.size + s2.size + s3.size};
  return string{new_size, true}.append_unsafe(s1.as_tmp_string()).append_unsafe(s2.as_tmp_string()).append_unsafe(s3.as_tmp_string()).finish_append();
}

string str_concat(str_concat_arg s1, str_concat_arg s2, str_concat_arg s3, str_concat_arg s4) {
  const auto new_size{s1.size + s2.size + s3.size + s4.size};
  return string{new_size, true}
    .append_unsafe(s1.as_tmp_string())
    .append_unsafe(s2.as_tmp_string())
    .append_unsafe(s3.as_tmp_string())
    .append_unsafe(s4.as_tmp_string())
    .finish_append();
}

string str_concat(str_concat_arg s1, str_concat_arg s2, str_concat_arg s3, str_concat_arg s4, str_concat_arg s5) {
  const auto new_size{s1.size + s2.size + s3.size + s4.size + s5.size};
  return string{new_size, true}
    .append_unsafe(s1.as_tmp_string())
    .append_unsafe(s2.as_tmp_string())
    .append_unsafe(s3.as_tmp_string())
    .append_unsafe(s4.as_tmp_string())
    .append_unsafe(s5.as_tmp_string())
    .finish_append();
}
