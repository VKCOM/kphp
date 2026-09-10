// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstdarg>
#include <cstdio>

#include "runtime-common/core/utils/kphp-assert-core.h"

// Standalone tests report notices without accessing the K2 logging platform.
void php_notice(const char* message, ...) {
  va_list args;
  va_start(args, message);
  static_cast<void>(std::vfprintf(stderr, message, args));
  va_end(args);
  static_cast<void>(std::fputc('\n', stderr));
}
