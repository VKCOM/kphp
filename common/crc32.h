// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __CRC32_H__
#define __CRC32_H__

#include <stdint.h>

#include "common/crc32_generic.h"

#define CRC32_POLY 0x04c11db7u
#define CRC32_REFLECTED_POLY 0xedb88320u

constexpr long long CRC32_REFLECTED_X319 = 0x9570d49500000000;
constexpr long long CRC32_REFLECTED_X255 = 0x01b5fd1d00000000;
constexpr long long CRC32_REFLECTED_X191 = 0x65673b4600000000;
constexpr long long CRC32_REFLECTED_X127 = 0x9ba54c6f00000000;
constexpr long long CRC32_REFLECTED_X95 = 0xccaa009e00000000;
constexpr long long CRC32_REFLECTED_X63 = 0xb8bc676500000000;

constexpr long long CRC32_REFLECTED_POLY_33_BIT = 0x1db710641;
constexpr long long CRC32_REFLECTED_MU = 0x1f7011641;

constexpr long long CRC64_REFLECTED_X319 = 0x60095b008a9efa44;
constexpr long long CRC64_REFLECTED_X191 = 0xe05dd497ca393ae4;
constexpr long long CRC64_REFLECTED_X255 = 0x3be653a30fe1af51;
constexpr long long CRC64_REFLECTED_X127 = 0xdabe95afc7875f40;
constexpr long long CRC64_REFLECTED_X95 = 0x1dee8a5e222ca1dc;
constexpr long long CRC64_REFLECTED_POLY_65_BIT = 0x92d8af2baf0e1e85;

extern uint64_t crc64_power_buf[126] __attribute__((aligned(16)));
void crc64_init_power_buf();

unsigned gf32_mul(unsigned a, unsigned b) __attribute__((pure));
unsigned gf32_mulx(unsigned a) __attribute__((pure));
uint64_t gf64_mul(uint64_t a, uint64_t b) __attribute__((pure));
uint64_t gf64_mulx(uint64_t a) __attribute__((pure));

typedef unsigned (*crc32_partial_func_t)(const void* data, long len, unsigned crc);
typedef unsigned (*crc32_combine_func_t)(unsigned crc1, unsigned crc2, long len2);
typedef uint64_t (*crc64_partial_func_t)(const void* data, long len, uint64_t crc);
typedef uint64_t (*crc64_combine_func_t)(uint64_t crc1, uint64_t crc2, int64_t len2);

extern crc32_partial_func_t crc32_partial;
extern crc64_partial_func_t crc64_partial;
extern crc32_combine_func_t compute_crc32_combine;
extern crc64_combine_func_t compute_crc64_combine;

uint64_t crc64_partial_one_table(const void* data, long len, uint64_t crc);
unsigned compute_crc32_combine_generic(unsigned crc1, unsigned crc2, long len2);
uint64_t compute_crc64_combine_generic(uint64_t crc1, uint64_t crc2, int64_t len2);

static inline unsigned compute_crc32(const void* data, long len) {
  return crc32_partial(data, len, -1) ^ -1;
}

static inline uint64_t compute_crc64(const void* data, long len) {
  return crc64_partial(data, len, -1LL) ^ -1LL;
}

/* crc32_check_and_repair returns
   0 : Cyclic redundancy check is ok
   1 : Cyclic redundancy check fails, but we fix one bit in input
   2 : Cyclic redundancy check fails, but we fix one bit in input_crc32
  -1 : Cyclic redundancy check fails, no repair possible.
       In this case *input_crc32 will be equal crc32 (input, l)

  Case force_exit == 1 (case 1, 2: kprintf call, case -1: assert fail).
*/
int crc32_check_and_repair(void* input, int l, unsigned* input_crc32, int force_exit);
int crc32_find_corrupted_bit(int size, unsigned d);
int crc32_repair_bit(unsigned char* input, int l, int k);

#endif
