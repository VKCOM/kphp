// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>

#include "common/stats/provider.h"
#include "common/wrappers/string_view.h"

struct binlog_reader_stats : private vk::not_copyable {
  static binlog_reader_stats& get() noexcept {
    static binlog_reader_stats stats;
    return stats;
  }

  std::chrono::seconds next_binlog_wait_time{std::chrono::seconds::zero()};
  vk::string_view next_binlog_expectator_name;
};
