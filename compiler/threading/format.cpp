#include "compiler/threading/format.h"

#include <cstdio>
#include <cstdarg>
#include <cstring>

#include "compiler/threading/tls.h"

using format_buff_t = char[5000];
static TLS<format_buff_t> format_buff;

char *format(char const *msg, ...) {
  va_list args;

  va_start (args, msg);
  format_buff_t local_buff;
  int cnt_chars = vsnprintf(local_buff, sizeof(format_buff_t), msg, args);
  assert(cnt_chars >= 0);
  va_end (args);

  format_buff_t &s = *format_buff;
  memcpy(s, local_buff, cnt_chars + 1);

  return s;
}
