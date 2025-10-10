// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"

Optional<string> f$vk_stats_hll_merge(const array<mixed>& a) noexcept;
Optional<double> f$vk_stats_hll_count(const string& hll) noexcept;
Optional<string> f$vk_stats_hll_create(const array<mixed>& a = array<mixed>(), int64_t size = (1 << 8)) noexcept;
Optional<string> f$vk_stats_hll_add(const string& hll, const array<mixed>& a) noexcept;
Optional<string> f$vk_stats_hll_pack(const string& hll) noexcept;
Optional<string> f$vk_stats_hll_unpack(const string& hll) noexcept;
bool f$vk_stats_hll_is_packed(const string& hll) noexcept;
