// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "kphp-core/kphp_core.h"

Optional<string> f$vk_stats_hll_merge(const array<mixed> &a);
Optional<double> f$vk_stats_hll_count(const string &hll);
Optional<string> f$vk_stats_hll_create(const array<mixed> &a = array<mixed>(), int64_t size = (1 << 8));
Optional<string> f$vk_stats_hll_add(const string &hll, const array<mixed> &a);
Optional<string> f$vk_stats_hll_pack(const string &hll);
Optional<string> f$vk_stats_hll_unpack(const string &hll);
bool f$vk_stats_hll_is_packed(const string &hll);
