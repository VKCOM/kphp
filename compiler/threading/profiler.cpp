#include "compiler/threading/profiler.h"

#include <algorithm>

TLS<Profiler> profiler;

static void profiler_print(ProfilerId id, const char *desc) {
  double total_time = 0;
  long long total_ticks = 0;
  long long total_count = 0;
  size_t total_memory = 0;
  for (int i = 0; i <= MAX_THREADS_COUNT; i++) {
    total_time += profiler.get(i)->raw[id].get_time();
    total_count += profiler.get(i)->raw[id].get_count();
    total_ticks += profiler.get(i)->raw[id].get_ticks();
    total_memory += profiler.get(i)->raw[id].get_memory();
  }
  if (total_count > 0) {
    if (total_ticks > 0) {
      fprintf(
        stderr, "%40s:\t%lf %lld %lld\n",
        desc, total_time, total_count, total_ticks / std::max(1ll, total_count)
      );
    }
    if (total_memory > 0) {
      fprintf(
        stderr, "%40s:\t%.5lfMb %lld %.5lf\n",
        desc, (double)total_memory / (1 << 20), total_count, (double)total_memory / total_count
      );
    }
  }
}


void profiler_print_all() {
  #define PRINT_PROF(x) profiler_print (PROF_E(x), #x);
  FOREACH_PROF (PRINT_PROF);
}
