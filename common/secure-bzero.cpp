// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/secure-bzero.h"

void secure_bzero(void *s, size_t n) {
  auto *p = reinterpret_cast<volatile unsigned char *>(s);

  while (n--) {
    *p++ = 0;
  }
}
