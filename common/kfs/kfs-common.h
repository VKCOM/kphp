// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef ENGINE_KFS_COMMON_H
#define ENGINE_KFS_COMMON_H

#include <stdbool.h>
#include <sys/cdefs.h>

#include "common/crypto/aes256.h"

#include "common/kfs/kfs-layout.h"
#include "common/kfs/kfs-typedefs.h"

#define MAX_FNAME_LEN 127

struct kfs_replica {
  kfs_hash_t replica_id_hash;
  const char* replica_prefix;
  int replica_prefix_len;
  int binlog_num;
  struct kfs_file_info** binlogs;
  // If both `old_binlog_files_path` and `new_binlog_files_limit` are non-zero,
  // at most `new_binlog_files_limit` binlog files kept in their location;
  // on rotations, older ones are moved into `old_binlog_files_path`.
  const char* old_binlog_files_path;
  int new_binlog_files_limit;
  int snapshot_num;
  struct kfs_file_info** snapshots;
  struct kfs_file_info* enc;
  vk_aes_ctx_t* ctx_crypto;
};

enum {
  KFS_FILE_SNAPSHOT = 1 << 0,
  KFS_FILE_TEMP = 1 << 1,
  KFS_FILE_NOT_FIRST = 1 << 2,
  KFS_FILE_SYMLINK = 1 << 3,
  KFS_FILE_ZIPPED = 1 << 4,
  KFS_FILE_ENCRYPTED = 1 << 5,
  KFS_FILE_REPLICATOR = 1 << 6,
  KFS_FILE_SNAPSHOT_DIFF = 1 << 7
};

struct kfs_file_info {
  struct kfs_replica* replica;
  int refcnt;
  int kfs_file_type;
  char* filename;
  int filename_len;
  int flags;          // +8 = symlink, +1 = snapshot, +2 = temp, +4 = not_first, +16 = zipped
  const char* suffix; // points to ".123456.bin" in "data3/messages006.123456.bin"
  kfs_hash_t file_hash;
  long long file_size;        // -1 = unknown; may change afterwards
  long long log_pos;          // -1 = unknown (read from LEV_ROTATE_FROM)
  long long min_log_pos;      // calculated by suffix (see kfs_classify_suffix)
  long long max_log_pos;      // calculated by suffix
  long long snapshot_log_pos; // -1 = unknown  // NOTE can't store its value in log_pos. There are some usages of log_pos for Snapshot in replicator.
  struct kfs_file_header* khdr;
  char* start; // pointer to first preloaded_bytes of this file
  unsigned char* iv;
  int preloaded_bytes;
  int kfs_headers;
  int mtime;
  int inode;
  int device;
};

static inline int kfs_file_info_add_ref(kfs_file_info_t* FI) {
  return __sync_add_and_fetch(&FI->refcnt, 1);
}

struct kfs_file {
  struct kfs_file_info* info;
  int fd;   // -1 = not open
  int lock; // 0 = unlocked, >=1 = read lock(s), -1 = write lock
  long long offset;
};

int preload_file_info_ex(struct kfs_file_info* FI) __attribute__((warn_unused_result));
bool preload_file_info(struct kfs_file_info* FI);

#endif // ENGINE_KFS_COMMON_H
