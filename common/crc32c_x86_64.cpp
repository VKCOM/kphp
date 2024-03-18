// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <assert.h>

#include "common/cpuid.h"
#include "common/crc32c.h"
#include "common/simd-vector.h"
#include "common/wrappers/likely.h"

#define CRC32C_REFLECTED_X1023 0x7417153fll
#define CRC32C_REFLECTED_X2047 0x1426a815ll
#define CRC32C_REFLECTED_X4095 0xe986c148ll
#define CRC32C_REFLECTED_X8191 0xcdc220ddll
#define CRC32C_REFLECTED_X16383 0x1acaec54ll

static unsigned crc32c_partial_sse42(const void *data, long len, unsigned crc) {
  const char *p = reinterpret_cast<const char *>(data);
  unsigned long long c = crc;
  while ((((uintptr_t)p) & 7) && (len > 0)) {
    asm volatile("crc32b (%2), %1\n\t" : "=r"(c) : "0"(c), "r"(p));
    p++;
    len--;
  }
  while (len >= 8) {
    asm volatile("crc32q (%2), %1\n\t" : "=r"(c) : "0"(c), "r"(p));
    p += 8;
    len -= 8;
  }
  while (len > 0) {
    asm volatile("crc32b (%2), %1\n\t" : "=r"(c) : "0"(c), "r"(p));
    p++;
    len--;
  }
  return c;
}

static unsigned crc32c_partial_sse42_clmul(const void *data, long len, unsigned crc) {
  const char *p = reinterpret_cast<const char *>(data);
  if (unlikely((((uintptr_t)p) & 1)) && (len > 0)) {
    asm volatile("crc32b (%2), %1\n\t" : "=r"(crc) : "0"(crc), "r"(p));
    p++;
    len--;
  }

  if (unlikely((((uintptr_t)p) & 2)) && (len >= 2)) {
    asm volatile("crc32w (%2), %1\n\t" : "=r"(crc) : "0"(crc), "r"(p));
    p += 2;
    len -= 2;
  }

  if ((((uintptr_t)p) & 4) && (len >= 4)) {
    asm volatile("crc32l (%2), %1\n\t" : "=r"(crc) : "0"(crc), "r"(p));
    p += 4;
    len -= 4;
  }

  if (unlikely(((uintptr_t)p) & 7)) {
    while (len > 0) {
      asm volatile("crc32b (%2), %1\n\t" : "=r"(crc) : "0"(crc), "r"(p));
      p++;
      len--;
    }
    return crc;
  }

  unsigned long long c = crc;

  while (len >= 3 * 0x400) {
    unsigned long long c1 = 0, c2 = 0;
    const char *q = p + 0x400;
    do {
      asm volatile("crc32q (%3), %0\n\t"
                   "crc32q 0x400(%3), %1\n\t"
                   "crc32q 0x800(%3), %2\n\t"
                   "crc32q 8(%3), %0\n\t"
                   "crc32q 0x408(%3), %1\n\t"
                   "crc32q 0x808(%3), %2\n\t"
                   : "=r"(c), "=r"(c1), "=r"(c2)
                   : "r"(p), "0"(c), "1"(c1), "2"(c2));
      p += 16;
    } while (p < q);
    static const v2di K = {CRC32C_REFLECTED_X16383, CRC32C_REFLECTED_X8191};
    v2di D, E;
    asm volatile("movq %1, %0\n\t" : "=x"(D) : "g"(c));
    asm volatile("movq %1, %0\n\t" : "=x"(E) : "g"(c1));
    v2di F = __builtin_ia32_pclmulqdq128(K, D, 0x00) ^ __builtin_ia32_pclmulqdq128(K, E, 0x01);
    asm volatile("movq %1, %0\n\t" : "=g"(c1) : "x"(F));
    unsigned int r;
    asm volatile("crc32l %1, %0\n\t" : "=r"(r) : "r"((int)c1), "0"(0));
    p += 0x800;
    len -= 3 * 0x400;
    c = r ^ (c1 >> 32) ^ c2;
  }
  while (len >= 0x180) {
    unsigned long long c1 = 0, c2 = 0;
    const char *q = p + 0x80;
    do {
      asm volatile("crc32q (%3), %0\n\t"
                   "crc32q 0x80(%3), %1\n\t"
                   "crc32q 0x100(%3), %2\n\t"
                   "crc32q 8(%3), %0\n\t"
                   "crc32q 0x88(%3), %1\n\t"
                   "crc32q 0x108(%3), %2\n\t"
                   : "=r"(c), "=r"(c1), "=r"(c2)
                   : "r"(p), "0"(c), "1"(c1), "2"(c2));
      p += 16;
    } while (p < q);
    static const v2di K = {CRC32C_REFLECTED_X2047, CRC32C_REFLECTED_X1023};
    v2di D, E;
    asm volatile("movq %1, %0\n\t" : "=x"(D) : "g"(c));
    asm volatile("movq %1, %0\n\t" : "=x"(E) : "g"(c1));
    v2di F = __builtin_ia32_pclmulqdq128(K, D, 0x00) ^ __builtin_ia32_pclmulqdq128(K, E, 0x01);
    asm volatile("movq %1, %0\n\t" : "=g"(c1) : "x"(F));
    unsigned int r;
    asm volatile("crc32l %1, %0\n\t" : "=r"(r) : "r"((int)c1), "0"(0));
    p += 0x100;
    len -= 0x180;
    c = r ^ (c1 >> 32) ^ c2;
  }
  while (len >= 32) {
    asm volatile("crc32q (%2), %1\n\t"
                 "crc32q 8(%2), %1\n\t"
                 "crc32q 0x10(%2), %1\n\t"
                 "crc32q 0x18(%2), %1\n\t"
                 : "=r"(c)
                 : "0"(c), "r"(p));
    p += 32;
    len -= 32;
  }
  while (len >= 8) {
    asm volatile("crc32q (%2), %1\n\t" : "=r"(c) : "0"(c), "r"(p));
    p += 8;
    len -= 8;
  }
  crc = c;
  if (len & 4) {
    asm volatile("crc32l (%2), %1\n\t" : "=r"(crc) : "0"(crc), "r"(p));
    p += 4;
  }
  if (len & 2) {
    asm volatile("crc32w (%2), %1\n\t" : "=r"(crc) : "0"(crc), "r"(p));
    p += 2;
  }
  if (len & 1) {
    asm volatile("crc32b (%2), %1\n\t" : "=r"(crc) : "0"(crc), "r"(p));
  }
  return crc;
}

void crc32c_init() __attribute__((constructor(101)));
void crc32c_init() {
  const kdb_cpuid_t *p = kdb_cpuid();
  assert(p->type == KDB_CPUID_X86_64);

  if (p->x86_64.ecx & (1 << 20)) {
    crc32c_partial = crc32c_partial_sse42;
    if (p->x86_64.ecx & 2) {
      crc32c_partial = crc32c_partial_sse42_clmul;
    }
  } else {
    crc32c_partial = &crc32c_partial_four_tables;
  }
}
