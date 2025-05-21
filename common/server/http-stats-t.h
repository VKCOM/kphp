// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/stats/provider.h"

class http_stats_t : public stats_t {
public:
  http_stats_t() noexcept {
    now_time = time(nullptr);
  }

  void add_general_stat([[maybe_unused]] const char* key, [[maybe_unused]] const char* value_format, ...) noexcept final {
    // ignore
  }

protected:
  void add_stat([[maybe_unused]] char type, const char* key, double value) noexcept final {
    sb_printf(&sb, "%s%s\t", normalize_key(key, "%s", ""), stats_suffix);
    sb_printf(&sb, "%.3f\t", value);
    sb_printf(&sb, "%ld", now_time);
    sb_printf(&sb, "\n");
  }

  void add_stat([[maybe_unused]] char type, const char* key, long long value) noexcept final {
    sb_printf(&sb, "%s%s\t", normalize_key(key, "%s", ""), stats_suffix);
    sb_printf(&sb, "%lld\t", value);
    sb_printf(&sb, "%ld", now_time);
    sb_printf(&sb, "\n");
  }

  void add_stat_with_tag_type([[maybe_unused]] char type, const char* key, const char* type_tag, double value) noexcept override {
    sb_printf(&sb, "%s_%s%s\t", normalize_key(key, "%s", ""), type_tag, stats_suffix);
    sb_printf(&sb, "%.3f\t", value);
    sb_printf(&sb, "%ld", now_time);
    sb_printf(&sb, "\n");
  }

  void add_stat_with_tag_type([[maybe_unused]] char type, const char* key, const char* type_tag, long long int value) noexcept override {
    sb_printf(&sb, "%s_%s%s\t", normalize_key(key, "%s", ""), type_tag, stats_suffix);
    sb_printf(&sb, "%lld\t", value);
    sb_printf(&sb, "%ld", now_time);
    sb_printf(&sb, "\n");
  }

private:
  const char* stats_suffix{"{family=\"gauges\",dimension=\"gauge\"}"};
  time_t now_time;
};
