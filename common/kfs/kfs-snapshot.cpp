// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/kfs/kfs-snapshot.h"

#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "common/kprintf.h"
#include "common/options.h"

#include "common/kfs/kfs-common.h"
#include "common/kfs/kfs-internal.h"
#include "common/kfs/kfs.h"

kfs_file_handle_t open_snapshot(kfs_replica_handle_t r, int snapshot_index) {
  if (!r) {
    return 0;
  }
  assert(0 <= snapshot_index && snapshot_index < r->snapshot_num);

  if (r->snapshots[snapshot_index]->flags & KFS_FILE_TEMP) {
    return 0;
  }

  struct kfs_file_info *fi = r->snapshots[snapshot_index];

  int fd = preload_file_info_ex(fi);

  if (fd == -2) {
    return 0;
  }

  vkprintf(2, "trying to open snapshot file %s\n", fi->filename);

  if (fd < 0) {
    fd = open(fi->filename, O_RDONLY);
    if (fd < 0) {
      kprintf("cannot open snapshot file %s: %m\n", fi->filename);
      return 0;
    }
    assert(lseek(fd, sizeof(struct kfs_file_header) * fi->kfs_headers, SEEK_SET) == sizeof(struct kfs_file_header) * fi->kfs_headers);
  }

  if (lock_whole_file(fd, F_RDLCK) <= 0) {
    assert(close(fd) >= 0);
    kprintf("cannot lock snapshot file %s: %m\n", fi->filename);
    return 0;
  }

  auto *f = static_cast<kfs_file *>(malloc(sizeof(struct kfs_file)));
  assert(f);
  memset(f, 0, sizeof(*f));
  f->info = fi;
  f->fd = fd;
  f->lock = 1;
  f->offset = fi->kfs_headers * sizeof(struct kfs_file_header);
  kfs_file_info_add_ref(fi);

  return f;
}

kfs_file_handle_t open_recent_snapshot(kfs_replica_handle_t r) {
  if (!r) {
    return 0;
  }
  vkprintf(2, "opening last snapshot file\n");

  struct kfs_file *f = 0;
  int p = r->snapshot_num - 1;
  while (p >= 0 && (f = open_snapshot(r, p)) == 0) {
    --p;
  }
  return f;
}

kfs_file_handle_t open_main_snapshot(kfs_file_handle_t snapshot_diff) {
  if (!snapshot_diff) {
    return NULL;
  }

  //
  // find and open first NON-diff snapshot
  //
  kfs_replica_handle_t r = snapshot_diff->info->replica;
  kfs_file *f = nullptr;

  for (int p = r->snapshot_num - 1; p >= 0; --p) {
    if ((KFS_FILE_TEMP | KFS_FILE_SNAPSHOT_DIFF) & r->snapshots[p]->flags) {
      continue;
    }

    if ((f = open_snapshot(r, p))) {
      return f;
    }
  }

  return f;

  //
  // Useless to read parent_pos here: cant compare it with pos in parent snapshot
  // because we know nothing about format of parent snapshot.
  //
}

int close_snapshot(kfs_file_handle_t f, bool close_handle) {
  return kfs_close_file(f, close_handle);
}

void init_engine_snapshot_descr(struct engine_snapshot_descr *_descr, kfs_file_handle_t _snapshot) {
  if (_snapshot) {
    _descr->name = strdup(_snapshot->info->filename);
    _descr->size = _snapshot->info->file_size;
    _descr->modification_time = _snapshot->info->mtime;
  } else {
    _descr->name = NULL;
    _descr->size = 0;
    _descr->modification_time = 0;
  }
}
