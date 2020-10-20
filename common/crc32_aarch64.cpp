#include <assert.h>

#include "common/cpuid.h"
#include "common/crc32.h"

void crc32_init(void) __attribute__((constructor(101)));
void crc32_init(void) {
  const kdb_cpuid_t *p = kdb_cpuid();
  assert(p->type == KDB_CPUID_AARCH64);

  crc32_partial = crc32_partial_generic;
  crc64_partial = crc64_partial_one_table;
  compute_crc32_combine = compute_crc32_combine_generic;
  compute_crc64_combine = compute_crc64_combine_generic;
}
