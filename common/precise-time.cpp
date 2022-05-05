// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/precise-time.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <sys/time.h>
#include <ctime>
#include <unistd.h>

#include "common/wrappers/likely.h"

int now;
static int start_time;
thread_local double precise_now;

double get_utime_monotonic() {
  struct timespec T;
  double res = 0;
#if _POSIX_TIMERS
  assert(clock_gettime(CLOCK_MONOTONIC, &T) >= 0);
  res = T.tv_sec + static_cast<double>(T.tv_nsec) * 1e-9;
#else
#error "No high-precision clock"
  res = time();
#endif

  precise_now = std::max(precise_now, res);
  return res;
}

double get_network_time() {
  struct timespec tp;
  if (!clock_gettime(CLOCK_MONOTONIC, &tp)) {
    precise_now = std::max(precise_now, tp.tv_sec + (1e-9) * tp.tv_nsec);
  }

  return precise_now;
}

long long get_precise_time() {
  timespec T;
  int r = clock_gettime(CLOCK_REALTIME, &T);
  assert(r >= 0);
  double res = static_cast<double>(T.tv_sec) + static_cast<double>(T.tv_nsec) * 1e-9;
  return static_cast<long long>(res * (1LL << 32));
}

void init_uptime() {
  start_time = time(nullptr);
}

int get_uptime() {
  if (unlikely(start_time == 0)) {
    init_uptime();
  }
  return now - start_time;
}
