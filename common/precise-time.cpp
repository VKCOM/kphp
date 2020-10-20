#include "common/precise-time.h"

#include <algorithm>
#include <assert.h>
#include <cstdint>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "common/cycleclock.h"
#include "common/wrappers/likely.h"

int now;
thread_local double precise_now;
long long precise_time;
long long precise_time_rdtsc;

double get_utime_monotonic (void) {
  struct timespec T;
  double res;
#if _POSIX_TIMERS
  assert (clock_gettime (CLOCK_MONOTONIC, &T) >= 0);
  res = T.tv_sec + (double) T.tv_nsec * 1e-9;
#else
#error "No high-precision clock"
  res = time();
#endif
  precise_now = std::max(precise_now, res);
  return precise_now;
}

uint64_t get_ntime_mono(void) {
  struct timespec T;
  uint64_t res = 0;
  #if _POSIX_TIMERS
  assert (clock_gettime (CLOCK_MONOTONIC, &T) >= 0);
  res = T.tv_sec * 1000LL * 1000LL * 1000LL + T.tv_nsec;
  #else
  #error "No high-precision clock"
  res = time() * 1e9;
  #endif
  return res;
}

template<typename T>
static inline double get_cached_time(T get_time, int cycles_cache) {
  thread_local static double last_cached_time = -1;
  thread_local static uint64_t last_cycleclock;
  const auto cycleclock = cycleclock_now();

  if (cycleclock - last_cycleclock > cycles_cache) {
    last_cached_time = get_time(last_cached_time);
    last_cycleclock = cycleclock;
  }

  return last_cached_time;
}

double get_double_time(void) {
  auto get_time = [](double last_cached_time) {
    struct timeval tv;
    if (!gettimeofday(&tv, NULL)) {
      return tv.tv_sec + (1e-6) * tv.tv_usec;
    }

    return last_cached_time;
  };

  return get_cached_time(get_time, 1e6);
}

double get_network_time() {
  auto get_time = [](double last_cached_time) {
    struct timespec tp;
    if (!clock_gettime(CLOCK_MONOTONIC, &tp)) {
      return tp.tv_sec + (1e-9) * tp.tv_nsec;
    }

    return last_cached_time;
  };

  precise_now = std::max(precise_now, get_cached_time(get_time, 1e6));

  return precise_now;
}

double get_utime (int clock_id) {
  struct timespec T;
#if _POSIX_TIMERS
  assert (clock_gettime (clock_id, &T) >= 0);
  double res = T.tv_sec + (double) T.tv_nsec * 1e-9;
#else
#error "No high-precision clock"
  double res = time ();
#endif
  if (clock_id == CLOCK_REALTIME) {
    precise_time = (long long) (res * (1LL << 32));
    precise_time_rdtsc = cycleclock_now();
  }
  return res;
}

long long get_precise_time (unsigned precision) {
  unsigned long long diff = cycleclock_now() - precise_time_rdtsc;
  if (diff > precision) {
    get_utime (CLOCK_REALTIME);
  }
  return precise_time;
}

static int start_time;
void init_uptime(void) {
  start_time = time(NULL);
}

int get_uptime(void) {
  if (unlikely(start_time == 0)) {
    init_uptime();
  }
  return now - start_time;
}
