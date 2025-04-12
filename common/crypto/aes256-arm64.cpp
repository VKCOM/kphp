// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/crypto/aes256-arm64.h"

#include <assert.h>

#include "common/cpuid.h"

bool crypto_arm64_has_aes_extension() {
  const kdb_cpuid_t* cpuid = kdb_cpuid();
  assert(cpuid->type == KDB_CPUID_ARM64);

  return false;
}
