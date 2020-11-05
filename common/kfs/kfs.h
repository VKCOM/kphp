// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __KFS_H__
#define __KFS_H__

#include <assert.h>
#include <stdbool.h>

#include "common/kfs/kfs-common.h"
#include "common/kfs/kfs-replica.h"

int kfs_classify_suffix(const char *suffix, long long *MMPos, const char *filename);
int lock_whole_file(int fd, int mode);

int kfs_close_file(kfs_file_handle_t F, bool close_handle);

extern char *engine_replica_name, *engine_snapshot_replica_name;
extern kfs_replica_handle_t engine_replica, engine_snapshot_replica;
extern kfs_file_handle_t Snapshot, SnapshotDiff;
extern struct engine_snapshot_descr engine_snapshot_description;
extern struct engine_snapshot_descr engine_snapshot_diff_description;

int engine_preload_filelist(const char *main_replica_name, const char *aux_replica_name);

kfs_binlog_zip_header_t *kfs_get_binlog_zip_header(const struct kfs_file_info *FI);
int kfs_count_headers(const struct kfs_file_header *kfs_Hdr, int r, int strict_check);
int kfs_bz_get_chunks_no(long long orig_file_size);
int kfs_bz_compute_header_size(long long orig_file_size);
int kfs_bz_decode(const struct kfs_file *F, long long off, void *dst, int *dest_len, int *disk_bytes_read);

int kfs_file_compute_initialization_vector(struct kfs_file_info *FI);

/* for binlog/snapshot metafile decryption */
void kfs_buffer_crypt(kfs_file_handle_t F, void *buff, long long size, long long off);
/* for decryption snapshot in load_index functions */
long long kfs_read_file(kfs_file_handle_t F, void *buff, long long size);
#define kfs_read_file_assert(F, buff, size) \
  { \
    long long __saved_size_t1 = size; assert (kfs_read_file (F, buff, __saved_size_t1) == __saved_size_t1); \
  }

#endif
