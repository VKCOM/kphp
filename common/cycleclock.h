#ifndef COMMON_CYCLECLOCK_H
#define COMMON_CYCLECLOCK_H

#include <stdint.h>

static __inline__ uint64_t cycleclock_now() {
#if defined(__x86_64__)
  uint32_t hi, lo;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)lo) | (((uint64_t)hi) << 32);
#elif defined(__aarch64__)
  uint64_t virtual_timer_value;
  asm volatile("mrs %0, cntvct_el0" : "=r"(virtual_timer_value));

  return virtual_timer_value;
#else
#error "Unsupported arch"
#endif
}

#endif // COMMON_CYCLECLOCK_H
