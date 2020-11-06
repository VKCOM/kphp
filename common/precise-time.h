#ifndef __PRECISE_TIME_H__
#define __PRECISE_TIME_H__

#include <stdint.h>
#include <time.h>

/* net-event.h */
extern int now;
extern thread_local double precise_now;
double get_utime_monotonic ();

// get CLOCK_MONOTONIC nanoseconds integer
uint64_t get_ntime_mono();

/* common/server-functions.h */
double get_utime (int clock_id);
extern long long precise_time;  // (long long) (2^16 * precise unixtime)
extern long long precise_time_rdtsc; // when precise_time was obtained
long long get_precise_time (unsigned precision);

/* ??? */
double get_double_time ();

double get_network_time();

int get_uptime();
void init_uptime();

#endif
