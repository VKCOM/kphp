#pragma once

#include <syscall.h>
#include <unistd.h>

#ifndef MADV_FREE
#define MADV_FREE 8
#endif

#ifndef MADV_DONTDUMP
  #define MADV_DONTDUMP 16
#endif

static inline int our_madvise(void *addr, size_t len, int advice) {
  return (int)syscall(__NR_madvise, addr, len, advice);
}
