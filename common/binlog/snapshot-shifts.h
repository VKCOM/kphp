// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "common/binlog/binlog-snapshot.h"
#include "common/tl/methods/string.h"

inline static long long get_snapshot_log_pos(const struct kfs_file_info* info) {
  long long log_pos{-1};
  if (info->preloaded_bytes < 4) {
    return log_pos;
  }

  const auto magic{*reinterpret_cast<const int32_t*>(info->start)};
  if (magic == kphp::tl::PMEMCACHED_OLD_INDEX_MAGIC) {
    log_pos = *reinterpret_cast<const int64_t*>(info->start + 2 * sizeof(int32_t) + sizeof(int64_t)); // add offset of log_pos1
  } else if (magic == kphp::tl::BARSIC_SNAPSHOT_HEADER_MAGIC && info->preloaded_bytes >= kphp::tl::COMMON_HEADER_META_SIZE - sizeof(int32_t)) {
    const auto tl_body_len{*reinterpret_cast<const int64_t*>(info->start + sizeof(int32_t))};
    if (info->preloaded_bytes >= kphp::tl::COMMON_HEADER_META_SIZE + tl_body_len) {
      kphp::tl::BarsicSnapshotHeader bsh{};
      vk::tl::fetch_from_buffer(info->start + kphp::tl::COMMON_HEADER_META_SIZE, tl_body_len, bsh);
      log_pos = bsh.payload_offset;
    }
  } else {
    fprintf(stderr, "Unknown snapshot magic for file %s: %08x\n", info->filename, magic);
  }

  if (log_pos < info->min_log_pos || log_pos > info->max_log_pos) {
    fprintf(stderr, "filename %s info->min_log_pos %lld info->max_log_pos %lld log_pos %lld\n", info->filename, info->min_log_pos, info->max_log_pos, log_pos);
  }

  return log_pos;
}
