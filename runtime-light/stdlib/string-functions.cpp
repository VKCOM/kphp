#include "runtime-light/stdlib/string-functions.h"

#include "runtime-light/component/component.h"


void print(const char *s, size_t s_len) {
  Response &response = get_component_context()->response;
  response.output_buffers[response.current_buffer].append(s, s_len);
}

void print(const char *s) {
  print(s, strlen(s));
}

void print(const string_buffer &sb) {
  print(sb.buffer(), sb.size());
}

void print(const string &s) {
  print(s.c_str(), s.size());
}

void f$debug_print_string(const string &s) {
  printf("debug_print_string [");
  for (int i = 0; i < s.size(); ++i) {
    printf("%d, ", s.c_str()[i]);
  }
  printf("]\n");
}

Optional<int64_t> f$byte_to_int(const string &s) {
  if (s.size() != 1) {
    php_warning("Cannot convert non-byte string to int");
    return false;
  }
  return *s.c_str();
}

Optional<string> f$int_to_byte(int64_t v) {
  if (v > 127 || v < -128) {
    php_warning("Cannot convert too big int to byte %ld", v);
    return false;
  }
  char c = v;
  string result(&c, 1);
  return result;
}

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
  auto new_size = s1.size() + s2.size();
  return string(new_size, true).append_unsafe(s1).append_unsafe(s2).finish_append();
}

string str_concat(str_concat_arg s1, str_concat_arg s2) {
  auto new_size = s1.size + s2.size;
  return string(new_size, true).append_unsafe(s1.as_tmp_string()).append_unsafe(s2.as_tmp_string()).finish_append();
}

string str_concat(str_concat_arg s1, str_concat_arg s2, str_concat_arg s3) {
  auto new_size = s1.size + s2.size + s3.size;
  return string(new_size, true).append_unsafe(s1.as_tmp_string()).append_unsafe(s2.as_tmp_string()).append_unsafe(s3.as_tmp_string()).finish_append();
}

string str_concat(str_concat_arg s1, str_concat_arg s2, str_concat_arg s3, str_concat_arg s4) {
  auto new_size = s1.size + s2.size + s3.size + s4.size;
  return string(new_size, true).append_unsafe(s1.as_tmp_string()).append_unsafe(s2.as_tmp_string()).append_unsafe(s3.as_tmp_string()).append_unsafe(s4.as_tmp_string()).finish_append();
}

string str_concat(str_concat_arg s1, str_concat_arg s2, str_concat_arg s3, str_concat_arg s4, str_concat_arg s5) {
  auto new_size = s1.size + s2.size + s3.size + s4.size + s5.size;
  return string(new_size, true).append_unsafe(s1.as_tmp_string()).append_unsafe(s2.as_tmp_string()).append_unsafe(s3.as_tmp_string()).append_unsafe(s4.as_tmp_string()).append_unsafe(s5.as_tmp_string()).finish_append();
}

