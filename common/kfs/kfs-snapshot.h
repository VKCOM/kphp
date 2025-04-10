// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef ENGINE_KFS_SNAPSHOT_H
#define ENGINE_KFS_SNAPSHOT_H

#include <stdbool.h>

#include "common/kfs/kfs-typedefs.h"

#define MAGIC_SNAPSHOT_DIFF_HEADER 0xc66607dc

struct engine_snapshot_descr {
  const char* name;
  long long size;
  int modification_time;
};

kfs_file_handle_t open_snapshot(kfs_replica_handle_t R, int snapshot_index); // index must be in [0, R->snapshot_num)
kfs_file_handle_t open_recent_snapshot(kfs_replica_handle_t Replica);        // file position is after kfs headers
kfs_file_handle_t open_main_snapshot(kfs_file_handle_t snapshot_diff);
int close_snapshot(kfs_file_handle_t F, bool close_handle);
void init_engine_snapshot_descr(struct engine_snapshot_descr* _descr, kfs_file_handle_t _snapshot);
#endif // ENGINE_KFS_SNAPSHOT_H
