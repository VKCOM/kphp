// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __PRECISE_TIME_H__
#define __PRECISE_TIME_H__

#include <cstdint>
#include <ctime>

/* net-event.h */
extern int now;
extern thread_local double precise_now;
double get_utime_monotonic ();

long long get_precise_time();
double get_network_time();

int get_uptime();
void init_uptime();

#endif
