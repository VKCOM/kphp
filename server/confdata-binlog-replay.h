#pragma once
#include <re2/re2.h>

#include "common/stats/provider.h"

#include "runtime/allocator.h"

void set_confdata_binlog_mask(const char *mask) noexcept;

void set_confdata_memory_limit(dl::size_type memory_limit) noexcept;
void set_confdata_blacklist_pattern(std::unique_ptr<re2::RE2> &&key_blacklist_pattern) noexcept;

void init_confdata_binlog_reader() noexcept;

void confdata_binlog_update_cron() noexcept;

void write_confdata_stats_to(stats_t *stats) noexcept;
