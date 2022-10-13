// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <ctime>

/* Wall Clock time */
extern int now;

/* Monotonic time */
extern thread_local double precise_now;

double get_precise_time(unsigned precision) noexcept;

static inline long long precise_time_to_binlog_time(double precise_time) noexcept {
  return static_cast<long long>(precise_time * (1LL << 32));
}

double get_utime_monotonic() noexcept;
double get_network_time() noexcept;

int get_uptime();
void init_uptime();
