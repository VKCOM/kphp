// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <assert.h>

#include "common/cpuid.h"
#include "common/crc32c.h"

void __attribute__((constructor(101))) crc32c_init() {
  const kdb_cpuid_t* p = kdb_cpuid();
  assert(p->type == KDB_CPUID_ARM64);

  crc32c_partial = crc32c_partial_four_tables;
}
