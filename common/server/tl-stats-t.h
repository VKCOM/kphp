// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/stats/provider.h"

class tl_stats_t : public stats_t {
public:
  void add_general_stat(const char *key, const char *value_format, va_list ap) noexcept final {
    sb_printf(&sb, "%s\t", key);
    vsb_printf(&sb, value_format, ap);
    sb_printf(&sb, "\n");
  }

  bool need_aggr_stats() noexcept final {
    return true;
  }

protected:
  void add_stat(char type [[maybe_unused]], const char *key, double value) noexcept final {
    sb_printf(&sb, "%s\t", key);
    sb_printf(&sb, "%.3f", value);
    sb_printf(&sb, "\n");
  }

  void add_stat(char type [[maybe_unused]], const char *key, long long value) noexcept final {
    sb_printf(&sb, "%s\t", key);
    sb_printf(&sb, "%lld", value);
    sb_printf(&sb, "\n");
  }
};
