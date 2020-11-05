// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __CRC32C_H__
#define __CRC32C_H__

#include <assert.h>
#include <stddef.h>

#include "common/crc32.h"

typedef enum crc32_type {
  CRC32, // Deprecated. Don't use this in new code.
  CRC32C
} crc32_type_t;

//extern unsigned int crc32c_table[256];
extern crc32_partial_func_t crc32c_partial;
unsigned compute_crc32c_combine (unsigned crc1, unsigned crc2, long len2);

static inline unsigned compute_crc32c (const void *data, int len) {
  return crc32c_partial (data, len, -1) ^ -1;
}

unsigned crc32c_partial_four_tables (const void *data, long len, unsigned crc);

static inline crc32_partial_func_t get_crc32_partial_func (crc32_type_t crc32_type) {
  switch (crc32_type) {
    case CRC32:
      return crc32_partial;
    case CRC32C:
      return crc32c_partial;
    default:
      assert (0 && "Unknown crc32_type");
  }

  return NULL;
}

static inline crc32_combine_func_t get_crc32_combine_func (crc32_type_t crc32_type) {
  switch (crc32_type) {
    case CRC32:
      return compute_crc32_combine;
    case CRC32C:
      return compute_crc32c_combine;
    default:
      assert (0 && "Unknown crc32_type");
  }

  return NULL;
}

#endif
