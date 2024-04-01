// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __CPUID_H__
#define __CPUID_H__

enum kdb_cpuid_type { KDB_CPUID_UNKNOWN = 0, KDB_CPUID_X86_64 = 0x280147b8, KDB_CPUID_AARCH64 = 0xfd327130, KDB_CPUID_ARM64 = 0x5d43a917 };
typedef enum kdb_cpuid_type kdb_cpuid_type_t;

typedef struct {
  kdb_cpuid_type_t type;
  union {
    struct {
      int ebx, ecx, edx;
    } x86_64;
  };
} kdb_cpuid_t;

const kdb_cpuid_t *kdb_cpuid ();

#endif
