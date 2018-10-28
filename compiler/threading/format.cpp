#include "compiler/threading/format.h"

#include <cstdio>
#include <stdarg.h>

#include "compiler/threading/tls.h"

typedef char pstr_buff_t[5000];
TLS<pstr_buff_t> pstr_buff;

char *bicycle_dl_pstr(char const *msg, ...) {
  pstr_buff_t &s = *pstr_buff;
  va_list args;

  va_start (args, msg);
  vsnprintf(s, 5000, msg, args);
  va_end (args);

  return s;
}
