// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/crc32.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "common/cpuid.h"
#include "common/kprintf.h"
#include "common/crc32_generic.h"


crc32_partial_func_t crc32_partial;
crc32_combine_func_t compute_crc32_combine;

/******************** CRC-64 ********************/

static const uint64_t crc64_table[256] = {
 0x0000000000000000LL, 0xb32e4cbe03a75f6fLL, 0xf4843657a840a05bLL, 0x47aa7ae9abe7ff34LL,
 0x7bd0c384ff8f5e33LL, 0xc8fe8f3afc28015cLL, 0x8f54f5d357cffe68LL, 0x3c7ab96d5468a107LL,
 0xf7a18709ff1ebc66LL, 0x448fcbb7fcb9e309LL, 0x0325b15e575e1c3dLL, 0xb00bfde054f94352LL,
 0x8c71448d0091e255LL, 0x3f5f08330336bd3aLL, 0x78f572daa8d1420eLL, 0xcbdb3e64ab761d61LL,
 0x7d9ba13851336649LL, 0xceb5ed8652943926LL, 0x891f976ff973c612LL, 0x3a31dbd1fad4997dLL,
 0x064b62bcaebc387aLL, 0xb5652e02ad1b6715LL, 0xf2cf54eb06fc9821LL, 0x41e11855055bc74eLL,
 0x8a3a2631ae2dda2fLL, 0x39146a8fad8a8540LL, 0x7ebe1066066d7a74LL, 0xcd905cd805ca251bLL,
 0xf1eae5b551a2841cLL, 0x42c4a90b5205db73LL, 0x056ed3e2f9e22447LL, 0xb6409f5cfa457b28LL,
 0xfb374270a266cc92LL, 0x48190ecea1c193fdLL, 0x0fb374270a266cc9LL, 0xbc9d3899098133a6LL,
 0x80e781f45de992a1LL, 0x33c9cd4a5e4ecdceLL, 0x7463b7a3f5a932faLL, 0xc74dfb1df60e6d95LL,
 0x0c96c5795d7870f4LL, 0xbfb889c75edf2f9bLL, 0xf812f32ef538d0afLL, 0x4b3cbf90f69f8fc0LL,
 0x774606fda2f72ec7LL, 0xc4684a43a15071a8LL, 0x83c230aa0ab78e9cLL, 0x30ec7c140910d1f3LL,
 0x86ace348f355aadbLL, 0x3582aff6f0f2f5b4LL, 0x7228d51f5b150a80LL, 0xc10699a158b255efLL,
 0xfd7c20cc0cdaf4e8LL, 0x4e526c720f7dab87LL, 0x09f8169ba49a54b3LL, 0xbad65a25a73d0bdcLL,
 0x710d64410c4b16bdLL, 0xc22328ff0fec49d2LL, 0x85895216a40bb6e6LL, 0x36a71ea8a7ace989LL,
 0x0adda7c5f3c4488eLL, 0xb9f3eb7bf06317e1LL, 0xfe5991925b84e8d5LL, 0x4d77dd2c5823b7baLL,
 0x64b62bcaebc387a1LL, 0xd7986774e864d8ceLL, 0x90321d9d438327faLL, 0x231c512340247895LL,
 0x1f66e84e144cd992LL, 0xac48a4f017eb86fdLL, 0xebe2de19bc0c79c9LL, 0x58cc92a7bfab26a6LL,
 0x9317acc314dd3bc7LL, 0x2039e07d177a64a8LL, 0x67939a94bc9d9b9cLL, 0xd4bdd62abf3ac4f3LL,
 0xe8c76f47eb5265f4LL, 0x5be923f9e8f53a9bLL, 0x1c4359104312c5afLL, 0xaf6d15ae40b59ac0LL,
 0x192d8af2baf0e1e8LL, 0xaa03c64cb957be87LL, 0xeda9bca512b041b3LL, 0x5e87f01b11171edcLL,
 0x62fd4976457fbfdbLL, 0xd1d305c846d8e0b4LL, 0x96797f21ed3f1f80LL, 0x2557339fee9840efLL,
 0xee8c0dfb45ee5d8eLL, 0x5da24145464902e1LL, 0x1a083bacedaefdd5LL, 0xa9267712ee09a2baLL,
 0x955cce7fba6103bdLL, 0x267282c1b9c65cd2LL, 0x61d8f8281221a3e6LL, 0xd2f6b4961186fc89LL,
 0x9f8169ba49a54b33LL, 0x2caf25044a02145cLL, 0x6b055fede1e5eb68LL, 0xd82b1353e242b407LL,
 0xe451aa3eb62a1500LL, 0x577fe680b58d4a6fLL, 0x10d59c691e6ab55bLL, 0xa3fbd0d71dcdea34LL,
 0x6820eeb3b6bbf755LL, 0xdb0ea20db51ca83aLL, 0x9ca4d8e41efb570eLL, 0x2f8a945a1d5c0861LL,
 0x13f02d374934a966LL, 0xa0de61894a93f609LL, 0xe7741b60e174093dLL, 0x545a57dee2d35652LL,
 0xe21ac88218962d7aLL, 0x5134843c1b317215LL, 0x169efed5b0d68d21LL, 0xa5b0b26bb371d24eLL,
 0x99ca0b06e7197349LL, 0x2ae447b8e4be2c26LL, 0x6d4e3d514f59d312LL, 0xde6071ef4cfe8c7dLL,
 0x15bb4f8be788911cLL, 0xa6950335e42fce73LL, 0xe13f79dc4fc83147LL, 0x521135624c6f6e28LL,
 0x6e6b8c0f1807cf2fLL, 0xdd45c0b11ba09040LL, 0x9aefba58b0476f74LL, 0x29c1f6e6b3e0301bLL,
 0xc96c5795d7870f42LL, 0x7a421b2bd420502dLL, 0x3de861c27fc7af19LL, 0x8ec62d7c7c60f076LL,
 0xb2bc941128085171LL, 0x0192d8af2baf0e1eLL, 0x4638a2468048f12aLL, 0xf516eef883efae45LL,
 0x3ecdd09c2899b324LL, 0x8de39c222b3eec4bLL, 0xca49e6cb80d9137fLL, 0x7967aa75837e4c10LL,
 0x451d1318d716ed17LL, 0xf6335fa6d4b1b278LL, 0xb199254f7f564d4cLL, 0x02b769f17cf11223LL,
 0xb4f7f6ad86b4690bLL, 0x07d9ba1385133664LL, 0x4073c0fa2ef4c950LL, 0xf35d8c442d53963fLL,
 0xcf273529793b3738LL, 0x7c0979977a9c6857LL, 0x3ba3037ed17b9763LL, 0x888d4fc0d2dcc80cLL,
 0x435671a479aad56dLL, 0xf0783d1a7a0d8a02LL, 0xb7d247f3d1ea7536LL, 0x04fc0b4dd24d2a59LL,
 0x3886b22086258b5eLL, 0x8ba8fe9e8582d431LL, 0xcc0284772e652b05LL, 0x7f2cc8c92dc2746aLL,
 0x325b15e575e1c3d0LL, 0x8175595b76469cbfLL, 0xc6df23b2dda1638bLL, 0x75f16f0cde063ce4LL,
 0x498bd6618a6e9de3LL, 0xfaa59adf89c9c28cLL, 0xbd0fe036222e3db8LL, 0x0e21ac88218962d7LL,
 0xc5fa92ec8aff7fb6LL, 0x76d4de52895820d9LL, 0x317ea4bb22bfdfedLL, 0x8250e80521188082LL,
 0xbe2a516875702185LL, 0x0d041dd676d77eeaLL, 0x4aae673fdd3081deLL, 0xf9802b81de97deb1LL,
 0x4fc0b4dd24d2a599LL, 0xfceef8632775faf6LL, 0xbb44828a8c9205c2LL, 0x086ace348f355aadLL,
 0x34107759db5dfbaaLL, 0x873e3be7d8faa4c5LL, 0xc094410e731d5bf1LL, 0x73ba0db070ba049eLL,
 0xb86133d4dbcc19ffLL, 0x0b4f7f6ad86b4690LL, 0x4ce50583738cb9a4LL, 0xffcb493d702be6cbLL,
 0xc3b1f050244347ccLL, 0x709fbcee27e418a3LL, 0x3735c6078c03e797LL, 0x841b8ab98fa4b8f8LL,
 0xadda7c5f3c4488e3LL, 0x1ef430e13fe3d78cLL, 0x595e4a08940428b8LL, 0xea7006b697a377d7LL,
 0xd60abfdbc3cbd6d0LL, 0x6524f365c06c89bfLL, 0x228e898c6b8b768bLL, 0x91a0c532682c29e4LL,
 0x5a7bfb56c35a3485LL, 0xe955b7e8c0fd6beaLL, 0xaeffcd016b1a94deLL, 0x1dd181bf68bdcbb1LL,
 0x21ab38d23cd56ab6LL, 0x9285746c3f7235d9LL, 0xd52f0e859495caedLL, 0x6601423b97329582LL,
 0xd041dd676d77eeaaLL, 0x636f91d96ed0b1c5LL, 0x24c5eb30c5374ef1LL, 0x97eba78ec690119eLL,
 0xab911ee392f8b099LL, 0x18bf525d915feff6LL, 0x5f1528b43ab810c2LL, 0xec3b640a391f4fadLL,
 0x27e05a6e926952ccLL, 0x94ce16d091ce0da3LL, 0xd3646c393a29f297LL, 0x604a2087398eadf8LL,
 0x5c3099ea6de60cffLL, 0xef1ed5546e415390LL, 0xa8b4afbdc5a6aca4LL, 0x1b9ae303c601f3cbLL,
 0x56ed3e2f9e224471LL, 0xe5c372919d851b1eLL, 0xa26908783662e42aLL, 0x114744c635c5bb45LL,
 0x2d3dfdab61ad1a42LL, 0x9e13b115620a452dLL, 0xd9b9cbfcc9edba19LL, 0x6a978742ca4ae576LL,
 0xa14cb926613cf817LL, 0x1262f598629ba778LL, 0x55c88f71c97c584cLL, 0xe6e6c3cfcadb0723LL,
 0xda9c7aa29eb3a624LL, 0x69b2361c9d14f94bLL, 0x2e184cf536f3067fLL, 0x9d36004b35545910LL,
 0x2b769f17cf112238LL, 0x9858d3a9ccb67d57LL, 0xdff2a94067518263LL, 0x6cdce5fe64f6dd0cLL,
 0x50a65c93309e7c0bLL, 0xe388102d33392364LL, 0xa4226ac498dedc50LL, 0x170c267a9b79833fLL,
 0xdcd7181e300f9e5eLL, 0x6ff954a033a8c131LL, 0x28532e49984f3e05LL, 0x9b7d62f79be8616aLL,
 0xa707db9acf80c06dLL, 0x14299724cc279f02LL, 0x5383edcd67c06036LL, 0xe0ada17364673f59LL
};

crc64_partial_func_t crc64_partial;
crc64_combine_func_t compute_crc64_combine;

uint64_t crc64_partial_one_table (const void *data, long len, uint64_t crc) {
  const auto *p = reinterpret_cast<const char *>(data);
  for (; len > 0; len--) {
    crc = crc64_table[(crc ^ *p++) & 0xff] ^ (crc >> 8);
  }
  return crc;    
}

/******************** GF-32 ********************/
unsigned gf32_mulx (unsigned a) {
  unsigned r = a >> 1;
  if (a & 1) {
    r ^= CRC32_REFLECTED_POLY;
  }
  return r;
}

unsigned gf32_mul (unsigned a, unsigned b) {
  unsigned x = 0;
  int i = 0;
  do {
    x = gf32_mulx (x);
    if (b & 1) {
      x ^= a;
    }
    b >>= 1;
  } while (++i < 32);
  return x;
}

#ifndef VKEXT
static unsigned gf32_pow (unsigned a, int k) {
  if (!k) { return 0x80000000; }
  unsigned x = gf32_pow (gf32_mul (a, a), k >> 1);
  if (k & 1) {
    x = gf32_mul (x, a);
  }
  return x;
}
#endif

static unsigned gf32_matrix_times (unsigned *matrix, unsigned vector) {
  unsigned sum = 0;
  while (vector) {
    if (vector & 1) {
      sum ^= *matrix;
    }
    vector >>= 1;
    matrix++;
  }
  return sum;
}

static void gf32_matrix_square (unsigned *square, unsigned *matrix) {
  int n = 0;
  do {
    square[n] = gf32_matrix_times (matrix, matrix[n]);
  } while (++n < 32);
}

unsigned compute_crc32_combine_generic (unsigned crc1, unsigned crc2, long len2) {
  static unsigned int crc32_power_buf[32*67];
  int n;
  /* degenerate case (also disallow negative lengths) */
  if (len2 <= 0) {
    return crc1;
  }
  if (!crc32_power_buf[32 * 67 - 1]) {
    crc32_power_buf[0] = CRC32_REFLECTED_POLY;
    for (n = 0; n < 31; n++) {
      crc32_power_buf[n+1] = 1U << n;
    }
    for (n = 1; n < 67; n++) {
      gf32_matrix_square (crc32_power_buf + (n << 5), crc32_power_buf + ((n - 1) << 5));
    }
    assert (crc32_power_buf[32 * 67 - 1]);
  }

  unsigned int *p = crc32_power_buf + 64;
  do {
    p += 32;
    if (len2 & 1) {
      crc1 = gf32_matrix_times (p, crc1);
    }
    len2 >>= 1;
  } while (len2);
  return crc1 ^ crc2;
}

/******************** GF-64 (reversed) ********************/

uint64_t gf64_mulx (uint64_t a) __attribute__ ((pure));
uint64_t gf64_mulx (uint64_t a) {
  uint64_t r = a >> 1;
  if (a & 1) {
    r ^= 0xc96c5795d7870f42ll;
  }
  return r;
}

uint64_t gf64_mul (uint64_t a, uint64_t b) {
  uint64_t x = 0;
  int i = 0;
  do {
    x = gf64_mulx (x);
    if (b & 1) {
      x ^= a;
    }
    b >>= 1;
  } while (++i < 64);
  return x;
}

uint64_t crc64_power_buf[126] __attribute__ ((aligned(16)));

void crc64_init_power_buf () {
  int n;
  uint64_t *p = crc64_power_buf;
  assert (!((uintptr_t) p & 15l));
  uint64_t a = 1ll << (63 - 7);
  const uint64_t b = 0xc96c5795d7870f42ll;
  for (n = 0; n < 63; n++) {
    p[0] = gf64_mul (a, b);
    p[1] = a;
    a = gf64_mulx (gf64_mul (a, a));
    p += 2;
  }
  p = crc64_power_buf;
  assert (p[3] == 1ll << (63 - 15));
  assert (p[4] == CRC64_REFLECTED_X95);
  assert (p[5] == 1ll << (63 - 31));
  assert (p[6] == CRC64_REFLECTED_X127);
  assert (p[7] == 1ll);
  assert (p[8] == CRC64_REFLECTED_X191);
  assert (p[9] == CRC64_REFLECTED_X127);
  assert (p[10] == CRC64_REFLECTED_X319);
  assert (p[11] == CRC64_REFLECTED_X255);
  assert (crc64_power_buf[125]);
}

uint64_t compute_crc64_combine_generic (uint64_t crc1, uint64_t crc2, int64_t len2) {
  if (len2 <= 0) {
    return crc1;
  }
  if (!crc64_power_buf[125]) {
    crc64_init_power_buf ();
  }

  int n = __builtin_ffsll (len2);
  uint64_t *p = crc64_power_buf + ((2 * (n - 1)) + 1);
  len2 >>= n;

  crc1 = gf64_mul (crc1, gf64_mulx (*p));

  while (len2) {
    p += 2;
    if (len2 & 1) {
      crc1 = gf64_mul (crc1, gf64_mulx (*p));
    }
    len2 >>= 1;
  }
  return crc1 ^ crc2;
}

/********************************* crc32 repair ************************/

#ifndef VKEXT

struct fcb_table_entry {
  unsigned p; //zeta ^ k
  int i;
};

static int cmp_fcb_table_entry (const void *a, const void *b) {
  const auto *x = reinterpret_cast<const fcb_table_entry *>(a);
  const auto *y = reinterpret_cast<const fcb_table_entry *>(b);
  if (x->p < y->p) { return -1; }
  if (x->p > y->p) { return  1; }
  if (x->i < y->i) { return -1; }
  if (x->i > y->i) { return  1; }
  return 0;
}

int crc32_find_corrupted_bit (int size, unsigned d) {
  int i, j;
  size += 4;
  int n = size << 3;
  int r = (int) (sqrt (n) + 0.5);
  vkprintf (3, "n = %d, r = %d, d = 0x%08x\n", n, r, d);
  auto *T = reinterpret_cast<fcb_table_entry *>(calloc (r, sizeof(fcb_table_entry)));
  assert (T != NULL);
  T[0].i = 0;
  T[0].p = 0x80000000u;
  for (i = 1; i < r; i++) {
    T[i].i = i;
    T[i].p = gf32_mulx (T[i-1].p);
  }
  assert (gf32_mulx (0xdb710641) == 0x80000000);
  qsort (T, r, sizeof (T[0]), cmp_fcb_table_entry);
  const unsigned q = gf32_pow (0xdb710641, r);

  unsigned A[32];
  A[31] = q;
  for (i = 30; i >= 0; i--) {
    A[i] = gf32_mulx (A[i+1]);
  }

  unsigned x = d;
  int max_j = n / r, res = -1;
  for (j = 0; j <= max_j; j++) {
    int a = -1, b = r;
    while (b - a > 1) {
      int c = ((a + b) >> 1);
      if (T[c].p <= x) { a = c; } else { b = c; }
    }
    if (a >= 0 && T[a].p == x) {
      res = T[a].i + r * j;
      break;
    }
    x = gf32_matrix_times (A, x);
  }
  free (T);
  return res;
}

int crc32_repair_bit (unsigned char *input, int l, int k) {
  if (k < 0) {
    return -1;
  }
  int idx = k >> 5, bit = k & 31, i = (l - 1) - (idx - 1) * 4;
  while (bit >= 8) {
    i--;
    bit -= 8;
  }
  if (i < 0) {
    return -2;
  }
  if (i >= l) {
    return -3;
  }
  int j = 7 - bit;
  input[i] ^= 1 << j;
  return 0;
}

int crc32_check_and_repair (void *input, int l, unsigned *input_crc32, int force_exit) {
  unsigned computed_crc32 = compute_crc32 (input, l);
  const unsigned crc32_diff = computed_crc32 ^ (*input_crc32);
  if (!crc32_diff) {
    return 0;
  }
  int k = crc32_find_corrupted_bit (l, crc32_diff);
  vkprintf (3, "find_corrupted_bit returns %d.\n", k);
  int r = crc32_repair_bit (reinterpret_cast<unsigned char *>(input), l, k);
  vkprintf (3, "repair_bit returns %d.\n", r);
  if (!r) {
    assert (compute_crc32 (input, l) == *input_crc32);
    if (force_exit) {
      kprintf ("crc32_check_and_repair successfully repair one bit in %d bytes block.\n", l);
    }
    return 1;
  }
  if (!(crc32_diff & (crc32_diff - 1))) { /* crc32_diff is power of 2 */
    *input_crc32 = computed_crc32;
    if (force_exit) {
      kprintf ("crc32_check_and_repair successfully repair one bit in crc32 (%d bytes block).\n", l);
    }
    return 2;
  }
  assert (!force_exit);
  *input_crc32 = computed_crc32;
  return -1;
}

#endif
