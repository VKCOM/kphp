// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdio>
#include <unistd.h>

#include "common/wrappers/likely.h"
#include "common/mixin/not_copyable.h"

extern int die_on_fail;

extern const char *engine_tag;
extern const char *engine_pid;

extern int php_disable_warnings;
extern int php_warning_level;
extern int php_warning_minimum_level;

void php_notice(char const *message, ...) __attribute__ ((format (printf, 1, 2)));
void php_warning(char const *message, ...) __attribute__ ((format (printf, 1, 2)));
void php_error(char const *message, ...) __attribute__ ((format (printf, 1, 2)));
void php_out_of_memory_warning(char const *message, ...) __attribute__ ((format (printf, 1, 2)));

template<class T>
class class_instance;
struct C$Throwable;
const char *php_uncaught_exception_error(const class_instance<C$Throwable> &ex) noexcept;

void php_assert__(const char *msg, const char *file, int line) __attribute__((noreturn));
void raise_php_assert_signal__();

#define php_assert(EX) do {                          \
  if (unlikely(!(EX))) {                             \
    php_assert__ (#EX, __FILE__, __LINE__);          \
  }                                                  \
} while(0)

#define php_critical_error(format, ...) do {                                                              \
  php_error ("Critical error \"" format "\" in file %s on line %d", ##__VA_ARGS__, __FILE__, __LINE__);   \
  raise_php_assert_signal__();                                                                            \
  fprintf (stderr, "_exiting in php_critical_error\n");                                                   \
  _exit (1);                                                                                              \
} while(0)
