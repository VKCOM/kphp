#ifndef KDB_COMMON_CACHELINE_H
#define KDB_COMMON_CACHELINE_H

#include <stdint.h>

#define KDB_CACHELINE_SIZE 64
#define KDB_CACHELINE_ALIGNED __attribute__((aligned(KDB_CACHELINE_SIZE)))

#endif // KDB_COMMON_CACHELINE_H
