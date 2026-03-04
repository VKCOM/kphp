// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <stdint.h>

#if defined(__ARM_ARCH_7A__) || defined(__aarch64__)
#define KDB_CACHELINE_SIZE 128
#else
#define KDB_CACHELINE_SIZE 64
#endif // __ARM_ARCH_7A__ || __aarch64__
#define KDB_CACHELINE_ALIGNED __attribute__((aligned(KDB_CACHELINE_SIZE)))
