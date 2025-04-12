// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/cpuid.h"

#include <assert.h>

const kdb_cpuid_t* kdb_cpuid() {
  static kdb_cpuid_t cached = {.type = KDB_CPUID_UNKNOWN};

#if defined(__x86_64__)
  if (cached.type != KDB_CPUID_UNKNOWN) {
    assert(cached.type == KDB_CPUID_X86_64);
    return &cached;
  }
  int a;
  asm volatile("cpuid\n\t" : "=a"(a), "=b"(cached.x86_64.ebx), "=c"(cached.x86_64.ecx), "=d"(cached.x86_64.edx) : "0"(1));
  cached.type = KDB_CPUID_X86_64;
#elif defined(__arm64__) // Apple M1
  if (cached.type) {
    assert(cached.type == KDB_CPUID_ARM64);
    return &cached;
  }

  cached.type = KDB_CPUID_ARM64;
#elif defined(__aarch64__)
  if (cached.type) {
    assert(cached.type == KDB_CPUID_AARCH64);
    return &cached;
  }

  cached.type = KDB_CPUID_AARCH64;
#else
#error "Unsupported arch"
#endif

  return &cached;
}
