#pragma once

#include "runtime/integer_types.h"
#include "runtime/kphp_core.h"

#pragma pack(push, 4)
struct sample_t {
  int max_size;
  //int size;            //assert(size <= max_size);
  int dataset_size;       //assert(size == min(dataset_size,max_size));
  int values[0];
};
#pragma pack(pop)


double f$vk_stats_merge_deviation(int64_t n1, Long sum1, double nsigma21, int64_t n2, Long sum2, double nsigma22);
double f$vk_stats_add_deviation(int64_t n, Long sum, double nsigma2, int64_t val);
Optional<string> f$vk_stats_decompress_sample(const string &s);
Optional<string> f$vk_stats_merge_samples(const array<mixed> &a);
Optional<array<int64_t>> f$vk_stats_parse_sample(const string &str);
Optional<string> f$vk_stats_hll_merge(const array<mixed> &a);
Optional<double> f$vk_stats_hll_count(const string &hll);
Optional<string> f$vk_stats_hll_create(const array<mixed> &a = array<mixed>(), int64_t size = (1 << 8));
Optional<string> f$vk_stats_hll_add(const string &hll, const array<mixed> &a);
Optional<string> f$vk_stats_hll_pack(const string &hll);
Optional<string> f$vk_stats_hll_unpack(const string &hll);
bool f$vk_stats_hll_is_packed(const string &hll);
