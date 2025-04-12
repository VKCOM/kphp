// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/server/init-snapshot.h"

#include <getopt.h>
#include <stdlib.h>

#include "common/binlog/kdb-binlog-common.h"
#include "common/kfs/kfs-snapshot.h"
#include "common/kfs/kfs.h"

#include "common/kprintf.h"
#include "common/options.h"
#include "common/precise-time.h"
#include "common/server/engine-settings.h"
#include "common/server/main-binlog.h"
#include "common/stats/provider.h"

static double index_load_time;

void engine_default_load_index(const char *name) {
  if (engine_preload_filelist(name, nullptr) < 0) {
    kprintf("cannot open binlog files for %s\n", name);
    exit(1);
  }

  vkprintf(3, "engine_preload_filelist done\n");

  //Snapshot reading
  SnapshotDiff = NULL;
  Snapshot = open_recent_snapshot(engine_snapshot_replica);
  if (Snapshot && (KFS_FILE_SNAPSHOT_DIFF & Snapshot->info->flags)) {
    SnapshotDiff = Snapshot;
    // open 'main' snapshot
    Snapshot = open_main_snapshot(SnapshotDiff);

    vkprintf(0, "snapshot: main %s, diff: %s\n", (Snapshot)?Snapshot->info->filename:"none", (SnapshotDiff)?SnapshotDiff->info->filename:"none");
  }

  if (get_engine_settings()->load_index) {
    init_engine_snapshot_descr(&engine_snapshot_description, Snapshot);
    init_engine_snapshot_descr(&engine_snapshot_diff_description, SnapshotDiff);

    index_load_time = -get_utime_monotonic();

    int res = get_engine_settings()->load_index();

    if (res < 0) {
      kprintf("load_index returned fail code %d.\n", res);
      exit(1);
    }
    index_load_time += get_utime_monotonic();
    if (Snapshot) {
      kprintf("Loaded snapshot %s at binlog position %lld in %f seconds\n", Snapshot->info->filename, jump_log_pos, index_load_time);
    } else {
      kprintf("No snapshot found, reading full binlog\n");
    }
  }
  if (Snapshot) {
    close_snapshot(Snapshot, true);
    Snapshot = NULL;
  }
}

