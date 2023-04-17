// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/mbstring.h"

#include "common/unicode/unicode-utils.h"
#include "common/unicode/utf8-utils.h"

bool mb_UTF8_check(const char *s) {
  do {
#define CHECK(condition) if (!(condition)) {return false;}
    unsigned int a = (unsigned char)(*s++);
    if ((a & 0x80) == 0) {
      if (a == 0) {
        return true;
      }
      continue;
    }

    CHECK ((a & 0x40) != 0);

    unsigned int b = (unsigned char)(*s++);
    CHECK((b & 0xc0) == 0x80);
    if ((a & 0x20) == 0) {
      CHECK((a & 0x1e) > 0);
      continue;
    }

    unsigned int c = (unsigned char)(*s++);
    CHECK((c & 0xc0) == 0x80);
    if ((a & 0x10) == 0) {
      int x = (((a & 0x0f) << 6) | (b & 0x20));
      CHECK(x != 0 && x != 0x360);//surrogates
      continue;
    }

    unsigned int d = (unsigned char)(*s++);
    CHECK((d & 0xc0) == 0x80);
    if ((a & 0x08) == 0) {
      int t = (((a & 0x07) << 6) | (b & 0x30));
      CHECK(0 < t && t < 0x110);//end of unicode
      continue;
    }

    return false;
#undef CHECK
  } while (true);

  php_assert (0);
}