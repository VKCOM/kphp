#include "runtime/kphp_core.h"
#include "_runtime-stubs.h"
#include "common/wrappers/fmt_format.h"
#include <benchmark/benchmark.h>

constexpr double zero_double_v = 0.12345;
constexpr double small_double_v = 1.2;
constexpr double mid_double_v = 123.4567;
constexpr double long_double_v = 123456789.123456789;

static void BM_fmt_new(benchmark::State& state, double d) {
  for(auto _ : state) {
    string str{d, string::NewAlgo{}};
    benchmark::DoNotOptimize(&str);
  }
}

BENCHMARK_CAPTURE(BM_fmt_new, zero_double, zero_double_v);
BENCHMARK_CAPTURE(BM_fmt_new, small_double, small_double_v);
BENCHMARK_CAPTURE(BM_fmt_new, mid_double, mid_double_v);
BENCHMARK_CAPTURE(BM_fmt_new, long_double, long_double_v);

static void BM_fmt_old(benchmark::State& state, double d) {
  for(auto _ : state) {
    string str{d, string::OldAlgo{}};
    benchmark::DoNotOptimize(&str);
  }
}

BENCHMARK_CAPTURE(BM_fmt_old, zero_double, zero_double_v);
BENCHMARK_CAPTURE(BM_fmt_old, small_double, small_double_v);
BENCHMARK_CAPTURE(BM_fmt_old, mid_double, mid_double_v);
BENCHMARK_CAPTURE(BM_fmt_old, long_double, long_double_v);

BENCHMARK_MAIN();
