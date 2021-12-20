// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>
#include <optional>
#include <vector>

#include "common/stats/provider.h"

char *engine_default_prepare_stats_with_tag_mask(stats_t &&stats, int *len, const char *statsd_prefix, unsigned int tag_mask);
char *engine_default_prepare_stats(stats_t &&stats, int *len, const char *statsd_prefix);
const char *engine_default_char_stats();

void engine_default_tl_stat_function(const std::optional<std::vector<std::string>> &sorted_filter_keys);
extern int (*tl_stat_function)(const std::optional<std::vector<std::string>> &sorted_filter_keys);
