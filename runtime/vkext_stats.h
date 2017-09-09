#pragma once

#include "runtime/integer_types.h"
#include "runtime/kphp_core.h"

#pragma pack(push,4)
struct sample {
  int max_size;
  //int size;            //assert(size <= max_size);
  int dataset_size;       //assert(size == min(dataset_size,max_size));
  int values[0];
};
#pragma pack(pop)
typedef struct sample sample_t;


double f$vk_stats_merge_deviation (int n1, Long sum1, double nsigma21, int n2, Long sum2, double nsigma22);
double f$vk_stats_add_deviation (int n, Long sum, double nsigma2, long long val);
OrFalse<string> f$vk_stats_decompress_sample (const string& s);
OrFalse<string> f$vk_stats_merge_samples (const array<var>& a);
OrFalse< array<int> > f$vk_stats_parse_sample (const string &str);
OrFalse<string> f$vk_stats_hll_merge (const array<var>& a);
OrFalse<double> f$vk_stats_hll_count (const string &s);
OrFalse<string> f$vk_stats_hll_create (const array<var> &a = array<var>(), int size = (1 << 8));
OrFalse<string> f$vk_stats_hll_add (string hll, const array<var> &a);
