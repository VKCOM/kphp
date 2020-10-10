#pragma once

#include "runtime/integer_types.h"
#include "runtime/kphp_core.h"


Optional<string> f$vk_stats_hll_merge(const array<mixed> &a);
Optional<double> f$vk_stats_hll_count(const string &hll);
Optional<string> f$vk_stats_hll_create(const array<mixed> &a = array<mixed>(), int64_t size = (1 << 8));
Optional<string> f$vk_stats_hll_add(const string &hll, const array<mixed> &a);
Optional<string> f$vk_stats_hll_pack(const string &hll);
Optional<string> f$vk_stats_hll_unpack(const string &hll);
bool f$vk_stats_hll_is_packed(const string &hll);
