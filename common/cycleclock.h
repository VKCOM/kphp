// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <iostream>

static __inline__ uint64_t cycleclock_now() {
#if defined(__x86_64__)
  uint32_t hi, lo;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return (static_cast<uint64_t>(lo)) | (static_cast<uint64_t>(hi) << 32);
#elif defined(__aarch64__)
  uint64_t virtual_timer_value;
  asm volatile("mrs %0, cntvct_el0" : "=r"(virtual_timer_value));

  return virtual_timer_value;
#else
  #error "Unsupported arch"
#endif
}

constexpr uint64_t INTEL_DEFAULT_FREQ_KHZ = 3200000;

/*
 * x86 timestamp counter ticks with the same frequency as CPU core does.
 * However, ARM platforms have a dedicated timer instead with its own
 * frequency (cntfrq_el0 returns 25MHz on an ARM KVM guest).
 * This leads us to *sigh* issues with precise-time caching since its
 * TTL is tied to a specific number of ticks spent. "cycleclock_freq" is
 * needed to shift TSC value, so it approximates whatever an Intel platform
 * could return under load.
 *
 * Next goes an excerpt from abseil-cpp source code:
 *
 * System timer of ARMv8 runs at a different frequency than the CPU's.
 * The frequency is fixed, typically in the range 1-50MHz.  It can be
 * read at CNTFRQ special register.  We assume the OS has set up
 * the virtual timer properly.
 */
static __inline__ uint64_t cycleclock_freq() {
#if defined(__x86_64__)
  return INTEL_DEFAULT_FREQ_KHZ / 1000;
#elif defined(__aarch64__)
  uint64_t aarch64_timer_frequency;
  asm volatile("mrs %0, cntfrq_el0" : "=r"(aarch64_timer_frequency));
  return aarch64_timer_frequency;
#else
  #error "Unsupported arch"
#endif
}
