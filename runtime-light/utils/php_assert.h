// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdio>
#include <unistd.h>

#include "common/wrappers/likely.h"
#include "common/mixin/not_copyable.h"

//#include "runtime-light/component/component.h"
//#include "runtime-light/utils/panic.h"

void panic();
void php_notice(char const *message, ...) __attribute__ ((format (printf, 1, 2)));
void php_warning(char const *message, ...) __attribute__ ((format (printf, 1, 2)));
void php_error(char const *message, ...) __attribute__ ((format (printf, 1, 2)));
void php_out_of_memory_warning(char const *message, ...) __attribute__ ((format (printf, 1, 2)));

template<class T>
class class_instance;
struct C$Throwable;
const char *php_uncaught_exception_error(const class_instance<C$Throwable> &ex) noexcept;

void php_assert__(const char *msg, const char *file, int line) __attribute__((noreturn));

#define php_assert(EX) do {                          \
  if (unlikely(!(EX))) {                             \
    php_assert__ (#EX, __FILE__, __LINE__);          \
  }                                                  \
} while(0)

#define php_critical_error(format, ...) do {                                                              \
  php_error ("Critical error \"" format "\" in file %s on line %d", ##__VA_ARGS__, __FILE__, __LINE__);   \
  panic();                                                                                                          \
} while(0)
