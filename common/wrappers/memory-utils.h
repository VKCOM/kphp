// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <sys/syscall.h>

#ifndef MADV_FREE
  #define MADV_FREE 8
#endif

#ifndef MADV_DONTDUMP
  #define MADV_DONTDUMP 16
#endif

static inline int our_madvise(void *addr, size_t len, int advice) {
  return (int)syscall(SYS_madvise, addr, len, advice);
}
