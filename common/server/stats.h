// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/stats/provider.h"

constexpr int STATS_BUFFER_LEN = 1 << 20;

char* get_engine_default_prepare_stats_buffer();
std::pair<char*, int> engine_default_prepare_stats_with_tag_mask(stats_t&& stats, const char* stats_prefix, unsigned int tag_mask);
char* engine_default_prepare_stats(stats_t&& stats, const char* stats_prefix);
const char* engine_default_char_stats();

void engine_default_tl_stat_function(const std::optional<std::vector<std::string>>& sorted_filter_keys);
extern int (*tl_stat_function)(const std::optional<std::vector<std::string>>& sorted_filter_keys);
