#include "compiler/threading/format.h"

#include <cstdio>
#include <stdarg.h>

#include "compiler/threading/tls.h"

typedef char format_buff_t[5000];
TLS<format_buff_t> format_buff;

char *format(char const *msg, ...) {
  format_buff_t &s = *format_buff;
  va_list args;

  va_start (args, msg);
  vsnprintf(s, 5000, msg, args);
  va_end (args);

  return s;
}
