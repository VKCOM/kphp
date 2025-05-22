// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/stats/provider.h"

class http_stats_t final : public stats_t {
public:
  http_stats_t() noexcept {
    now_time = time(nullptr);
  }

  void add_general_stat([[maybe_unused]] const char* key, [[maybe_unused]] const char* value_format, ...) noexcept override {
    // ignore
  }

protected:
  void add_stat(const char type, const char* key, const double value) noexcept override {
    const auto normalized_key = normalize_key(key, "%s", "");
    const auto normalized_type = to_open_metrics_type(type);

    sb_printf(&sb, "# TYPE %s %s\n", normalized_key, normalized_type);
    sb_printf(&sb, "%s%s\t", normalized_key, stats_suffix);
    sb_printf(&sb, "%.3f\t", value);
    sb_printf(&sb, "%ld", now_time);
    sb_printf(&sb, "\n");
  }

  void add_stat(const char type, const char* key, const long long value) noexcept override {
    const auto normalized_key = normalize_key(key, "%s", "");
    const auto normalized_type = to_open_metrics_type(type);

    sb_printf(&sb, "# TYPE %s %s\n", normalized_key, normalized_type);
    sb_printf(&sb, "%s%s\t", normalized_key, stats_suffix);
    sb_printf(&sb, "%lld\t", value);
    sb_printf(&sb, "%ld", now_time);
    sb_printf(&sb, "\n");
  }

  void add_stat_with_tag_type(const char type, const char* key, const char* type_tag, const double value) noexcept override {
    const auto normalized_key = normalize_key(key, "%s", "");
    const auto normalized_type = to_open_metrics_type(type);

    sb_printf(&sb, "# TYPE %s %s\n", normalized_key, normalized_type);
    sb_printf(&sb, "%s_%s%s\t", normalized_key, type_tag, stats_suffix);
    sb_printf(&sb, "%.3f\t", value);
    sb_printf(&sb, "%ld", now_time);
    sb_printf(&sb, "\n");
  }

  void add_stat_with_tag_type(const char type, const char* key, const char* type_tag, const long long int value) noexcept override {
    const auto normalized_key = normalize_key(key, "%s", "");
    const auto normalized_type = to_open_metrics_type(type);

    sb_printf(&sb, "# TYPE %s %s\n", normalized_key, normalized_type);
    sb_printf(&sb, "%s_%s%s\t", normalized_key, type_tag, stats_suffix);
    sb_printf(&sb, "%lld\t", value);
    sb_printf(&sb, "%ld", now_time);
    sb_printf(&sb, "\n");
  }

private:
  const char* stats_suffix{"{job=\"kphp_metrics\"}"};
  time_t now_time;

  static const char* to_open_metrics_type(const char type) noexcept {
    switch (type) {
    case 'g':
      return "gauge";
    case 'h':
      return "histogram";
    default:
      return "unknown";
    }
  }
};
