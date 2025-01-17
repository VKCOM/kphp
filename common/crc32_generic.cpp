// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/crc32_generic.h"

unsigned crc32_partial_generic (const void *data, long len, unsigned crc) {
  const int *p = (const int *) data;
  long x;
#define DO_ONE(v) crc ^= v; crc = crc32_table0[crc & 0xff] ^ crc32_table1[(crc & 0xff00) >> 8] ^ crc32_table2[(crc & 0xff0000) >> 16] ^ crc32_table[crc >> 24];
#define DO_FOUR(p) DO_ONE((p)[0]); DO_ONE((p)[1]); DO_ONE((p)[2]); DO_ONE((p)[3]);

  for (x = (len >> 5); x > 0; x--) {
    DO_FOUR (p);
    DO_FOUR (p + 4);
    p += 8;
  }
  if (len & 16) {
    DO_FOUR (p);
    p += 4;
  }
  if (len & 8) {
    DO_ONE (p[0]);
    DO_ONE (p[1]);
    p += 2;
  }
  if (len & 4) {
    DO_ONE (*p++);
  }
  /*
  for (x = (len >> 2) & 7; x > 0; x--) {
    DO_ONE (*p++);
  }
  */
#undef DO_ONE
#undef DO_FOUR
  const char *q = (const char *) p;
  if (len & 2) {
    crc = crc32_table[(crc ^ q[0]) & 0xff] ^ (crc >> 8);
    crc = crc32_table[(crc ^ q[1]) & 0xff] ^ (crc >> 8);
    q += 2;
  }
  if (len & 1) {
    crc = crc32_table[(crc ^ *q++) & 0xff] ^ (crc >> 8);
  }
  return crc;
}
