// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/utils/php_assert.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <ctime>
#include <cxxabi.h>
#include <execinfo.h>
#include <unistd.h>
#include <sys/wait.h>

#include "runtime-light/utils/panic.h"

static void php_warning_impl(bool out_of_memory, int error_type, char const *message, va_list args) {
  (void) out_of_memory;
  const int BUF_SIZE = 1000;
  char buf[BUF_SIZE];

  int size = vsnprintf(buf, BUF_SIZE, message, args);
  get_platform_context()->log(error_type, size, buf);
  if (error_type == Error) {
    panic();
  }
}

void php_debug(char const *message, ...) {
  va_list args;
  va_start(args, message);
  php_warning_impl(false, Debug, message, args);
  va_end(args);
}

void php_notice(char const *message, ...) {
  va_list args;
  va_start(args, message);
  php_warning_impl(false, Info, message, args);
  va_end(args);
}

void php_warning(char const *message, ...) {
  va_list args;
  va_start(args, message);
  php_warning_impl(false, Warn, message, args);
  va_end(args);
}

void php_error(char const *message, ...) {
  va_list args;
  va_start(args, message);
  php_warning_impl(false, Error, message, args);
  va_end(args);
}

void php_assert__(const char *msg, const char *file, int line) {
  php_error("Assertion \"%s\" failed in file %s on line %d", msg, file, line);
  panic();
  _exit(1);
}
