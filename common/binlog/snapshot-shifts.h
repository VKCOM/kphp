// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

inline static int get_snapshot_position_shift(const struct kfs_file_info *info) {
  if (info->preloaded_bytes < 4) {
    return 0xffff;
  }
  int magic = *(int *)(info->start);
  if (magic == 0x53407fa0) {    // PMEMCACHED_RAM_INDEX_MAGIC
    return 16;
  }
  fprintf(stderr, "Unknown snapshot magic for file %s: %08x\n", info->filename, magic);
  return 0xffff;
}

inline static long long get_snapshot_log_pos(const struct kfs_file_info *info) {
  int shift = get_snapshot_position_shift(info);
  long long log_pos = -1;
  if (info->preloaded_bytes >= shift + 8) {
    log_pos = *(long long *)(info->start + shift);
    if (!(info->min_log_pos <= log_pos && log_pos <= info->max_log_pos)) {
      fprintf(stderr, "filename %s info->min_log_pos %lld info->max_log_pos %lld log_pos %lld shift %d\n", info->filename, info->min_log_pos, info->max_log_pos, log_pos, shift);
      assert(info->min_log_pos <= log_pos && log_pos <= info->max_log_pos);
    }
  }
  return log_pos;
}

