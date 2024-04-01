// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <assert.h>

#include "common/cpuid.h"
#include "common/crc32.h"

void __attribute__((constructor(101))) crc32_init() {
  const kdb_cpuid_t *p = kdb_cpuid();
  assert(p->type == KDB_CPUID_AARCH64);

  crc32_partial = crc32_partial_generic;
  crc64_partial = crc64_partial_one_table;
  compute_crc32_combine = compute_crc32_combine_generic;
  compute_crc64_combine = compute_crc64_combine_generic;
}
