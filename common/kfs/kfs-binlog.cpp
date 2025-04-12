// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/kfs/kfs-binlog.h"

#include <cassert>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common/binlog/kdb-binlog-common.h"
#include "common/kprintf.h"

#include "common/kfs/kfs-common.h"
#include "common/kfs/kfs-internal.h"
#include "common/kfs/kfs.h"

static inline long long kfs_get_binlog_file_size(const struct kfs_file_info *FI) {
  if (!(FI->flags & KFS_FILE_ZIPPED)) {
    return FI->file_size - sizeof(struct kfs_file_header) * FI->kfs_headers;
  }
  const kfs_binlog_zip_header_t *H = kfs_get_binlog_zip_header(FI);
  assert (H);
  return H->orig_file_size;
}

kfs_file_handle_t open_binlog(const kfs_replica_t *R, long long log_pos) {
  if (!R) {
    return 0;
  }

  int l = -1, r = R->binlog_num;
  while (r - l > 1) { // Z[l].min_log_pos <= log_pos < Z[r].min_log_pos
    int m = (l + r) >> 1;
    if (R->binlogs[m]->min_log_pos <= log_pos) {
      l = m;
    } else {
      r = m;
    }
  }
  if (l < 0) {
    vkprintf(2, "asked for log_pos %lld, minimal possible log_pos %lld\n", log_pos, R->binlogs ? R->binlogs[0]->min_log_pos : -1);
    return 0;
  }

  int p = l;

  //while (p >= 0 && log_pos <= R->binlogs[p]->max_log_pos + R->binlogs[p]->file_size) {
  while (p >= 0) {
    struct kfs_file_info *FI = R->binlogs[p];
    /* This cutoff works only for not zipped binlogs.
       In the zipped binlog case, we must preload zipped binlog header for obtaining original file size.
    */
    if (!(FI->flags & KFS_FILE_ZIPPED) && (log_pos > FI->max_log_pos + FI->file_size)) {
      break;
    }
    int fd = preload_file_info_ex(FI);
    if (fd == -2) {
      vkprintf(2, "couldn't preload file info for %s: %m\n", FI->filename);
      --p;
      continue;
    }

    assert (FI->log_pos >= 0 && FI->file_size > 0);

    long long log_pos_end = FI->log_pos + kfs_get_binlog_file_size(FI);

    if (log_pos > log_pos_end) {
      if (fd >= 0) {
        assert (close(fd) >= 0);
      }
      vkprintf(2, "%s: log_pos (%lld) > log_pos_end (%lld)\n", FI->filename, log_pos, log_pos_end);
      break;
    }

    if (log_pos >= FI->log_pos) {
      /* found */
      vkprintf(2, "binlog position %lld found in file %s, covering %lld..%lld\n", log_pos, FI->filename, FI->log_pos, log_pos_end);
      if (fd < 0) {
        fd = open(FI->filename, O_RDONLY);
        if (fd < 0) {
          vkprintf(2, "cannot open binlog file %s: %m\n", FI->filename);
          return 0;
        }
      }

      long long file_pos = log_pos - FI->log_pos + sizeof(struct kfs_file_header) * FI->kfs_headers;
      //kprintf ("FI->file_size:%lld, FI->flags: %d, FI->kfs_headers:%d, file:%s, file_pos: %lld, log_pos: %lld\n", FI->file_size, FI->flags, FI->kfs_headers, FI->filename, file_pos, log_pos);
      assert (lseek(fd, file_pos, SEEK_SET) == file_pos);

      auto *F = static_cast<kfs_file *>(calloc(1, sizeof(struct kfs_file)));
      assert (F);
      F->info = FI;
      F->fd = fd;
      F->offset = FI->kfs_headers * sizeof(struct kfs_file_header);
      kfs_file_info_add_ref(FI);

      return F;
    }

    if (fd >= 0) {
      assert (close(fd) >= 0);
    }

    --p;
  }

  return 0;
}

kfs_file_handle_t next_binlog(kfs_file_handle_t F) {
  struct kfs_file_info *FI = F->info, *FI2;
  struct kfs_replica *R = FI->replica;

  assert (F->fd >= 0);
  assert (R);

  int l = -1, r = R->binlog_num;
  while (r - l > 1) { // Z[l].min_log_pos <= log_pos < Z[r].min_log_pos
    int m = (l + r) >> 1;
    if (R->binlogs[m]->min_log_pos <= FI->log_pos) {
      l = m;
    } else {
      r = m;
    }
  }

  while (l >= 0 && R->binlogs[l] != FI) {
    l--;
  }

  assert (l >= 0);

  long long log_pos = FI->log_pos + kfs_get_binlog_file_size(FI);

  vkprintf(3, "next_binlog(%p): FI=%p, R=%p, l=%d, binlogs=%d, log_pos=%lld\n", F, FI, R, l, R->binlog_num, log_pos);

  l++;
  if (l == R->binlog_num) {
    return 0;
  }

  FI2 = R->binlogs[l];
  int fd = preload_file_info_ex(FI2);

  if (fd == -2) {
    return 0;
  }

  if (FI2->log_pos != log_pos || log_pos <= 0) {
    return 0;
  }

  long long log_pos_end = log_pos + kfs_get_binlog_file_size(FI2);

  vkprintf(2, "next binlog file %s, covering %lld..%lld\n", FI2->filename, log_pos, log_pos_end);
  if (fd < 0) {
    fd = open(FI2->filename, O_RDONLY);
    if (fd < 0) {
      kprintf("cannot open binlog file %s: %m\n", FI2->filename);
      return 0;
    }
  }
  F = static_cast<kfs_file_handle_t>(malloc(sizeof(struct kfs_file)));
  assert (F);
  memset(F, 0, sizeof(*F));
  F->info = FI2;
  F->fd = fd;
  F->offset = FI2->kfs_headers * sizeof(struct kfs_file_header);
  kfs_file_info_add_ref(FI2);

  return F;
}

int close_binlog(kfs_file_handle_t F, bool close_handle) {
  return kfs_close_file(F, close_handle);
}
