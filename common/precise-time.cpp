// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/precise-time.h"

#include <algorithm>
#include <cstdint>
#include <chrono>

#include "common/cycleclock.h"
#include "common/wrappers/likely.h"

int now;
thread_local double precise_now;

const uint64_t DEFAULT_CYCLECLOCK_COEFFICIENT = 1;
static const uint64_t cycleclock_coefficient = std::max(INTEL_DEFAULT_FREQ_KHZ * 1000 / cycleclock_freq(), DEFAULT_CYCLECLOCK_COEFFICIENT);

template<double (*cb)()>
struct cached_time {
  double get_time(int cycles_cache) {
    const auto cycleclock = cycleclock_now() * cycleclock_coefficient;

    if (cycleclock - last_cycleclock > cycles_cache) {
      last_cached_time = cb();
      last_cycleclock = cycleclock;
    }

    return last_cached_time;
  }
  double last_cached_time = -1;
  uint64_t last_cycleclock{};
};

double get_utime_monotonic() noexcept {
  std::chrono::duration<double> seconds = std::chrono::steady_clock::now().time_since_epoch();
  precise_now = seconds.count();
  return precise_now;
}

double get_double_time_since_epoch() noexcept {
  std::chrono::duration<double> seconds = std::chrono::system_clock::now().time_since_epoch();
  return seconds.count();
}

double get_network_time() noexcept {
  thread_local static cached_time<get_utime_monotonic> cache{};
  return std::max(precise_now, cache.get_time(1e6));
}

double get_precise_time(unsigned precision) noexcept {
  thread_local static cached_time<get_double_time_since_epoch> cache{};
  return cache.get_time(precision);
}

static int start_time;

void init_uptime() {
  start_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

int get_uptime() {
  if (unlikely(start_time == 0)) {
    init_uptime();
  }
  return now - start_time;
}
