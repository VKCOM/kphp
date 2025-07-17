// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/server/stats.h"

#include "common/server/engine-settings.h"
#include "common/server/open-metrics-stats-t.h"
#include "common/server/tl-stats-t.h"
#include "common/stats/buffer.h"
#include "common/tl/parse.h"

int (*tl_stat_function)(const std::optional<std::vector<std::string>>& sorted_filter_keys);

static char buf[STATS_BUFFER_LEN];

char* get_engine_default_prepare_stats_buffer() {
  return buf;
}

std::pair<char*, int> engine_default_prepare_stats_with_tag_mask(stats_t&& stats, const char* stats_prefix, unsigned int tag_mask) {
  stats.stats_prefix = stats_prefix;

  sb_init(&stats.sb, buf, STATS_BUFFER_LEN);
  prepare_common_stats_with_tag_mask(&stats, tag_mask);

  return {buf, stats.sb.pos};
}

char* engine_default_prepare_stats(stats_t&& stats, const char* stats_prefix) {
  return engine_default_prepare_stats_with_tag_mask(std::move(stats), stats_prefix, stats_tag_mask_full).first;
}

void engine_default_tl_stat_function(const std::optional<std::vector<std::string>>& sorted_filter_keys) {
  tl_store_stats(engine_default_prepare_stats(tl_stats_t{}, NULL), 0, sorted_filter_keys);
}

const char* engine_default_char_stats() {
  return engine_default_prepare_stats(tl_stats_t{}, NULL);
}

std::pair<std::string_view, std::size_t> engine_default_open_metrics_stat_stats() {
  return engine_default_prepare_stats_with_tag_mask(open_metrics_stats_t{}, nullptr, stats_tag_mask_full);
}

__attribute__((constructor)) static void register_char_stats() {
  engine_settings_handlers.char_stats = engine_default_char_stats;
}
