// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <csignal>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cxxabi.h>
#include <execinfo.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>

#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/utils/logs.h"

static void php_warning_impl(bool out_of_memory, int error_type, char const* message, va_list args) {
  (void)out_of_memory;
  const int BUF_SIZE = 1000;
  char buf[BUF_SIZE];

  int size = vsnprintf(buf, BUF_SIZE, message, args);
  k2::log(error_type, size, buf);
  if (error_type == std::to_underlying(LogLevel::Error)) {
    critical_error_handler();
  }
}

void php_debug(char const* message, ...) {
  va_list args;
  va_start(args, message);
  php_warning_impl(false, std::to_underlying(LogLevel::Debug), message, args);
  va_end(args);
}

void php_notice(char const* message, ...) {
  va_list args;
  va_start(args, message);
  php_warning_impl(false, std::to_underlying(LogLevel::Info), message, args);
  va_end(args);
}

void php_warning(char const* message, ...) {
  va_list args;
  va_start(args, message);
  php_warning_impl(false, std::to_underlying(LogLevel::Warn), message, args);
  va_end(args);
}

void php_error(char const* message, ...) {
  va_list args;
  va_start(args, message);
  php_warning_impl(false, std::to_underlying(LogLevel::Error), message, args);
  va_end(args);
}

void php_assert__(const char* msg, const char* file, int line) {
  php_error("Assertion \"%s\" failed in file %s on line %d", msg, file, line);
  critical_error_handler();
  k2::exit(1);
}
