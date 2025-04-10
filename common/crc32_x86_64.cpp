// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <assert.h>

#include <x86intrin.h>

#include "common/cpuid.h"
#include "common/crc32.h"
#include "common/simd-vector.h"

#ifdef __clang__
__attribute__((constructor(101))) void crc32_init() {
  crc32_partial = crc32_partial_generic;
  crc64_partial = crc64_partial_one_table;
  compute_crc32_combine = compute_crc32_combine_generic;
  compute_crc64_combine = compute_crc64_combine_generic;
}
#else

/******************** CLMUL ********************/

// mu(65-bit): 01001110000111110010001100110110000010111001010010110001111010101
constexpr long long CRC64_REFLECTED_MU = 0x9c3e466c172963d5;

#define DO_FOUR(p)                                                                                                                                             \
  DO_ONE((p)[0]);                                                                                                                                              \
  DO_ONE((p)[1]);                                                                                                                                              \
  DO_ONE((p)[2]);                                                                                                                                              \
  DO_ONE((p)[3]);
#define DO_ONE(v)                                                                                                                                              \
  crc ^= v;                                                                                                                                                    \
  crc = crc32_table0[crc & 0xff] ^ crc32_table1[(crc & 0xff00) >> 8] ^ crc32_table2[(crc & 0xff0000) >> 16] ^ crc32_table[crc >> 24];

#ifdef __SANITIZE_ADDRESS__
#define attribute_no_sanitize_address __attribute__((no_sanitize_address))
#else
#define attribute_no_sanitize_address
#endif

static const char mask[64] __attribute__((aligned(16))) = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

static unsigned attribute_no_sanitize_address crc32_partial_clmul(const void* data, long len, unsigned crc) {
  if (len <= 40) {
    const int* p = reinterpret_cast<const int*>(data);
    if (len & 32) {
      DO_FOUR(p);
      DO_FOUR(p + 4);
      p += 8;
    }
    if (len & 16) {
      DO_FOUR(p);
      p += 4;
    }
    if (len & 8) {
      DO_ONE(p[0]);
      DO_ONE(p[1]);
      p += 2;
    }
    if (len & 4) {
      DO_ONE(*p++);
    }
    const char* q = (const char*)p;
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

  /* works only for len >= 32 */
  const char* q = (const char*)(((uintptr_t)data) & -16L);
  int o = 32 - ((uintptr_t)data & 15);

  v2di D = (*(v2di*)q), E = (*(v2di*)(q + 16)), C, G, H;
  asm volatile("movd %1, %0\n\t" : "=x"(C) : "g"(crc));
  asm volatile("pcmpeqw %0, %0\n\t" : "=x"(G)); // G := 2 ^ 128 - 1

  H = (v2di)__builtin_ia32_loaddqu(mask + o);
  G = (v2di)__builtin_ia32_pshufb128((v16qi)G, (v16qi)H);
  D ^= (v2di)__builtin_ia32_pshufb128((v16qi)C, (v16qi)H);
  D &= G;

  if (__builtin_expect(o <= 19, 0)) {
    E ^= (v2di)__builtin_ia32_pshufb128((v16qi)C, (v16qi)__builtin_ia32_loaddqu(mask + 16 + o));
  }

  len -= o;
  q += 32;

  static const v2di K128 = {CRC32_REFLECTED_X191, CRC32_REFLECTED_X127};
  static const v2di K256 = {CRC32_REFLECTED_X319, CRC32_REFLECTED_X255};

  while (len >= 32) {
    D = __builtin_ia32_pclmulqdq128(K256, D, 0x00) ^ *((v2di*)q) ^ __builtin_ia32_pclmulqdq128(K256, D, 0x11);
    E = __builtin_ia32_pclmulqdq128(K256, E, 0x00) ^ *((v2di*)(q + 16)) ^ __builtin_ia32_pclmulqdq128(K256, E, 0x11);
    len -= 32;
    q += 32;
  }

  if (len >= 16) {
    D = __builtin_ia32_pclmulqdq128(K256, D, 0x00) ^ __builtin_ia32_pclmulqdq128(K256, D, 0x11);
    E = __builtin_ia32_pclmulqdq128(K128, E, 0x00) ^ *((v2di*)q) ^ __builtin_ia32_pclmulqdq128(K128, E, 0x11);
    D ^= E;
    len -= 16;
    q += 16;
  } else {
    D = __builtin_ia32_pclmulqdq128(K128, D, 0x00) ^ E ^ __builtin_ia32_pclmulqdq128(K128, D, 0x11);
  }

  if (len > 0) {
    E = (v2di)__builtin_ia32_pshufb128((v16qi)D, __builtin_ia32_loaddqu(mask + 32 + len));
    H = (v2di)__builtin_ia32_loaddqu(mask + 16 + len);
    E ^= (v2di)__builtin_ia32_pshufb128(*((v16qi*)q), (v16qi)H);
    D = (v2di)__builtin_ia32_pshufb128((v16qi)D, (v16qi)H);
    D = __builtin_ia32_pclmulqdq128(K128, D, 0x00) ^ E ^ __builtin_ia32_pclmulqdq128(K128, D, 0x11);
  }

  static const v2di K64 = {CRC32_REFLECTED_X95, CRC32_REFLECTED_X63};

  D = (v2di)__builtin_ia32_pclmulqdq128(K64, D, 0x00) ^ __builtin_ia32_pslldqi128(__builtin_ia32_psrldqi128(D, 64), 32);

  D ^= (v2di)__builtin_ia32_pclmulqdq128(K64, D, 0x01);

  int tmp[4] __attribute__((aligned(16)));
#ifdef BARRETT_REDUCTION
  static const v2di MU = {CRC32_REFLECTED_MU, CRC32_REFLECTED_POLY_33_BIT};
  H = (v2di)__builtin_ia32_punpckhdq128((v4si)(C ^ C), (v4si)D);
  G = (v2di)__builtin_ia32_pclmulqdq128(MU, H, 0x00);
  H = (v2di)__builtin_ia32_pclmulqdq128(MU, G, 0x01);
  D ^= __builtin_ia32_pslldqi128(H, 32);
  asm volatile("movdqa %1, (%0)\n\t" : : "r"(&tmp), "x"(D) : "memory");
  return tmp[3];
#else
  asm volatile("movdqa %1, (%0)\n\t" : : "r"(&tmp), "x"(D) : "memory");
  crc = tmp[2];
  return crc32_table0[crc & 0xff] ^ crc32_table1[(crc & 0xff00) >> 8] ^ crc32_table2[(crc & 0xff0000) >> 16] ^ crc32_table[crc >> 24] ^ tmp[3];
#endif
}

#undef DO_ONE
#undef DO_FOUR

uint64_t crc64_barrett_reduction(v2di D) {
  static const v2di MU = {CRC64_REFLECTED_MU, CRC64_REFLECTED_POLY_65_BIT};
  /* After reflection mu constant is 64 bit */
  v2di E = __builtin_ia32_pclmulqdq128(MU, D, 0x00);
  /* The carry-less multiplication has to be performed with a PCLMULQDQ and an XOR operation
     since P(x) is 65 bit constant. */
  D ^= __builtin_ia32_pclmulqdq128(MU, E, 0x01) ^ __builtin_ia32_pslldqi128(E, 64);

  uint64_t tmp[2] __attribute__((aligned(16)));
  asm volatile("movdqa %1, (%0)\n\t" : : "r"(&tmp), "x"(D) : "memory");
  return tmp[1];
}

static uint64_t attribute_no_sanitize_address crc64_partial_clmul(const void* data, long len, uint64_t crc) {
  if (len <= 31) {
    return crc64_partial_one_table(data, len, crc);
  }

  /* works only for len >= 32 */
  const char* q = (const char*)(((uintptr_t)data) & -16L);
  int o = 32 - ((uintptr_t)data & 15);

  v2di D = (*(v2di*)q), E = (*(v2di*)(q + 16)), C, G, H;
  asm volatile("movq %1, %0\n\t" : "=x"(C) : "g"(crc));
  asm volatile("pcmpeqw %0, %0\n\t" : "=x"(G)); // G := 2 ^ 128 - 1

  H = (v2di)__builtin_ia32_loaddqu(mask + o);
  G = (v2di)__builtin_ia32_pshufb128((v16qi)G, (v16qi)H);
  D ^= (v2di)__builtin_ia32_pshufb128((v16qi)C, (v16qi)H);
  D &= G;

  if (o <= (32 - 9)) {
    E ^= (v2di)__builtin_ia32_pshufb128((v16qi)C, (v16qi)__builtin_ia32_loaddqu(mask + 16 + o));
  }

  len -= o;
  q += 32;

  static const v2di K128 = {CRC64_REFLECTED_X191, CRC64_REFLECTED_X127};
  static const v2di K256 = {CRC64_REFLECTED_X319, CRC64_REFLECTED_X255};

  while (len >= 32) {
    D = __builtin_ia32_pclmulqdq128(K256, D, 0x00) ^ *((v2di*)q) ^ __builtin_ia32_pclmulqdq128(K256, D, 0x11);
    E = __builtin_ia32_pclmulqdq128(K256, E, 0x00) ^ *((v2di*)(q + 16)) ^ __builtin_ia32_pclmulqdq128(K256, E, 0x11);
    len -= 32;
    q += 32;
  }

  if (len >= 16) {
    D = __builtin_ia32_pclmulqdq128(K256, D, 0x00) ^ __builtin_ia32_pclmulqdq128(K256, D, 0x11);
    E = __builtin_ia32_pclmulqdq128(K128, E, 0x00) ^ *((v2di*)q) ^ __builtin_ia32_pclmulqdq128(K128, E, 0x11);
    D ^= E;
    len -= 16;
    q += 16;
  } else {
    D = __builtin_ia32_pclmulqdq128(K128, D, 0x00) ^ E ^ __builtin_ia32_pclmulqdq128(K128, D, 0x11);
  }

  if (len > 0) {
    E = (v2di)__builtin_ia32_pshufb128((v16qi)D, __builtin_ia32_loaddqu(mask + 32 + len));
    H = (v2di)__builtin_ia32_loaddqu(mask + 16 + len);
    E ^= (v2di)__builtin_ia32_pshufb128(*((v16qi*)q), (v16qi)H);
    D = (v2di)__builtin_ia32_pshufb128((v16qi)D, (v16qi)H);
    D = __builtin_ia32_pclmulqdq128(K128, D, 0x00) ^ E ^ __builtin_ia32_pclmulqdq128(K128, D, 0x11);
  }

  D = (v2di)__builtin_ia32_pclmulqdq128(K128, D, 0x01) ^ __builtin_ia32_psrldqi128(D, 64);

  return crc64_barrett_reduction(D);
}

static unsigned compute_crc32_combine_clmul(unsigned crc1, unsigned crc2, long len2) {
  static unsigned int crc32_power_buf[63 * 4] __attribute__((aligned(16)));
  int n;
  if (len2 <= 0) {
    return crc1;
  }
  unsigned int* p;
  if (!crc32_power_buf[63 * 4 - 1]) {
    p = crc32_power_buf;
    assert(!((uintptr_t)p & 15l));
    unsigned a = 1 << (31 - 7);
    const unsigned b = gf32_mul(CRC32_REFLECTED_POLY, CRC32_REFLECTED_POLY);
    for (n = 0; n < 63; n++) {
      p[0] = 0;
      p[1] = gf32_mul(a, b);
      p[2] = 0;
      p[3] = a;
      a = gf32_mulx(gf32_mul(a, a));
      p += 4;
    }
    p = crc32_power_buf + 8;
    assert(*((uint64_t*)(p + 0)) == CRC32_REFLECTED_X95);
    assert(*((uint64_t*)(p + 4)) == CRC32_REFLECTED_X127);
    assert(*((uint64_t*)(p + 6)) == CRC32_REFLECTED_X63);
    assert(*((uint64_t*)(p + 8)) == CRC32_REFLECTED_X191);
    assert(*((uint64_t*)(p + 10)) == CRC32_REFLECTED_X127);
    assert(*((uint64_t*)(p + 12)) == CRC32_REFLECTED_X319);
    assert(*((uint64_t*)(p + 14)) == CRC32_REFLECTED_X255);
    assert(crc32_power_buf[63 * 4 - 1]);
  }

  v2di D;
  asm volatile("movd %1, %0\n\t" : "=x"(D) : "g"(crc1));
  D = __builtin_ia32_pslldqi128(D, 96);

  n = __builtin_ffsl(len2);
  p = crc32_power_buf + (4 * (n - 1));
  len2 >>= n;

  D = __builtin_ia32_pclmulqdq128(*((v2di*)p), D, 0x11);

  while (len2) {
    p += 4;
    if (len2 & 1) {
      v2di E = *((v2di*)p);
      D = __builtin_ia32_pclmulqdq128(E, D, 0x11) ^ __builtin_ia32_pclmulqdq128(E, D, 0x00);
    }
    len2 >>= 1;
  }

  int tmp[4] __attribute__((aligned(16)));
  D ^= (v2di)__builtin_ia32_pclmulqdq128(*((v2di*)(crc32_power_buf + 12)), D, 0x01);
  asm volatile("movdqa %1, (%0)\n\t" : : "r"(&tmp), "x"(D) : "memory");
  crc1 = tmp[2];
  return (crc32_table0[crc1 & 0xff] ^ crc32_table1[(crc1 & 0xff00) >> 8] ^ crc32_table2[(crc1 & 0xff0000) >> 16] ^ crc32_table[crc1 >> 24] ^ tmp[3]) ^ crc2;
}

static uint64_t compute_crc64_combine_clmul(uint64_t crc1, uint64_t crc2, int64_t len2) {
  if (len2 <= 0) {
    return crc1;
  }
  if (!crc64_power_buf[125]) {
    crc64_init_power_buf();
  }
  v2di D;
  asm volatile("movq %1, %0\n\t" : "=x"(D) : "g"(crc1));
  D = __builtin_ia32_pslldqi128(D, 64);

  int n = __builtin_ffsll(len2);
  uint64_t* p = crc64_power_buf + (2 * (n - 1));
  len2 >>= n;

  D = __builtin_ia32_pclmulqdq128(*((v2di*)p), D, 0x11);

  while (len2) {
    p += 2;
    if (len2 & 1) {
      v2di E = *((v2di*)p);
      D = __builtin_ia32_pclmulqdq128(E, D, 0x11) ^ __builtin_ia32_pclmulqdq128(E, D, 0x00);
    }
    len2 >>= 1;
  }
  return crc64_barrett_reduction(D) ^ crc2;
}

void __attribute__((constructor(101))) crc32_init() {
  const kdb_cpuid_t* p = kdb_cpuid();
  assert(p->type == KDB_CPUID_X86_64);

  if (p->x86_64.ecx & 2) {
    crc32_partial = crc32_partial_clmul;
    crc64_partial = crc64_partial_clmul;
    compute_crc32_combine = compute_crc32_combine_clmul;
    compute_crc64_combine = compute_crc64_combine_clmul;
  } else {
    crc32_partial = crc32_partial_generic;
    crc64_partial = crc64_partial_one_table;
    compute_crc32_combine = compute_crc32_combine_generic;
    compute_crc64_combine = compute_crc64_combine_generic;
  }
}
#endif
