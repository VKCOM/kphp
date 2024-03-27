#include "runtime-light/stdlib/string_functions.h"

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
