// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/server/stats.h"

#include "common/server/engine-settings.h"
#include "common/server/tl-stats-t.h"
#include "common/stats/buffer.h"

#include "common/tl/parse.h"

int (*tl_stat_function)(const std::optional<std::vector<std::string>> &sorted_filter_keys);

char *engine_default_prepare_stats_with_tag_mask(stats_t &&stats, int *len, const char *statsd_prefix, unsigned int tag_mask) {
  static char buf[1 << 20];
  stats.statsd_prefix = statsd_prefix;

  sb_init(&stats.sb, buf, 1 << 20);
  prepare_common_stats_with_tag_mask(&stats, tag_mask);

  if (len) {
    *len = stats.sb.pos;
  }
  return buf;
}

char *engine_default_prepare_stats(stats_t &&stats, int *len, const char *statsd_prefix) {
  return engine_default_prepare_stats_with_tag_mask(std::move(stats), len, statsd_prefix, STATS_TAG_MASK_FULL);
}

void engine_default_tl_stat_function(const std::optional<std::vector<std::string>> &sorted_filter_keys) {
  tl_store_stats(engine_default_prepare_stats(tl_stats_t{}, NULL, NULL), 0, sorted_filter_keys);
}

const char *engine_default_char_stats() {
  return engine_default_prepare_stats(tl_stats_t{}, NULL, NULL);
}

__attribute__((constructor))
static void register_char_stats() {
  engine_settings_handlers.char_stats = engine_default_char_stats;
}



