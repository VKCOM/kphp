// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/kfs/kfs-replica.h"

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common/kprintf.h"
#include "common/options.h"
#include "common/server/engine-settings.h"
#include "common/sha1.h"
#include "common/wrappers/pathname.h"

#include "common/kfs/kfs-binlog.h"
#include "common/kfs/kfs-common.h"
#include "common/kfs/kfs-internal.h"
#include "common/kfs/kfs-snapshot.h"
#include "common/kfs/kfs.h"

#define MAX_DIRNAME_LEN 127

#define MAX_REPLICA_FILES 8192

static struct kfs_file_info *t_binlogs[MAX_REPLICA_FILES], *t_snapshots[MAX_REPLICA_FILES];

static inline int cmp_file_info(const struct kfs_file_info *FI, const struct kfs_file_info *FJ) {
  if (FI->min_log_pos < FJ->min_log_pos) {
    return -1;
  } else if (FI->min_log_pos > FJ->min_log_pos) {
    return 1;
  } else {
    return strcmp(FI->filename, FJ->filename);
  }
}

/* storage-engine optimization (replacing O(n^2) sort by qsort) */
static int qsort_cmp_file_info(const void *x, const void *y) {
  return cmp_file_info(*(const struct kfs_file_info **)x, *(const struct kfs_file_info **)y);
}

static void kf_sort(struct kfs_file_info **T, int n) {
  qsort(T, n, sizeof(T[0]), qsort_cmp_file_info);
}

static int replica_create_crypto_context(kfs_replica_handle_t R) {
  if (R->ctx_crypto) {
    return 0;
  }
  return -1;
}

static enum kfs_file_type get_kfs_file_type(int name_class) {
  if (name_class == KFS_FILE_ENCRYPTED) {
    return kfs_enc;
  }
  switch (name_class & 3) {
    case 0:
    case 2:
      return kfs_binlog;
    case 1:
      return kfs_snapshot;
    case 3:
      return kfs_partial;
    default:
      kprintf("illegal name_class %d.\n", name_class);
  }
  return kfs_unknown;
}

kfs_replica_handle_t open_replica(const char *replica_name, int flags) {
  const char *after_slash = strrchr(replica_name, '/');
  static __thread char dirname[MAX_DIRNAME_LEN + 1 + MAX_FNAME_LEN + 1];
  int dirname_len, filename_len;

  struct kfs_file_info *t_enc = NULL;

  static __thread struct stat tmp_stat;

  if (after_slash) {
    after_slash++;
    dirname_len = after_slash - replica_name;
    assert(dirname_len < MAX_DIRNAME_LEN);
    memcpy(dirname, replica_name, dirname_len);
    dirname[dirname_len] = 0;
  } else {
    after_slash = replica_name;
    dirname_len = 2;
    dirname[0] = '.';
    dirname[1] = '/';
    dirname[2] = 0;
  }

  filename_len = strlen(after_slash);

  DIR *D = opendir(dirname);

  if (!D) {
    kprintf("kfs_open_replica: cannot open directory %s: %m\n", dirname);
    return 0;
  }

  int len, path_len, name_class, binlogs = 0, snapshots = 0;
  struct kfs_file_info *FI;
  struct kfs_replica *R = 0;

  if (flags & KFS_OPEN_REPLICA_FLAG_FORCE) {
    R = static_cast<kfs_replica *>(calloc(sizeof(*R), 1));
    assert(R);
    R->replica_prefix = strdup(replica_name);
    assert(R->replica_prefix);
    R->replica_prefix_len = strlen(replica_name);
  }

  while (true) {
    errno = 0;
    dirent *DE = readdir(D);
    if (errno) {
      dirname[dirname_len] = 0;
      kprintf("kfs_open_replica: error while reading directory %s: %m\n", dirname);
      close_replica(R);
      return nullptr;
    }
    if (!DE) {
      break;
    }
    if (DE->d_type != DT_REG && DE->d_type != DT_UNKNOWN) {
      continue;
    }
    len = strlen(DE->d_name);
    vkprintf(2, "See file %s\n", DE->d_name);
    if (len > MAX_FNAME_LEN) {
      continue;
    }
    if (len < filename_len || memcmp(DE->d_name, after_slash, filename_len)) {
      vkprintf(2, "Skip it, because doesn't start from %.*s\n", filename_len, after_slash);
      continue;
    }

    static long long MMPos[2];

    name_class = kfs_classify_suffix(DE->d_name + filename_len, MMPos, DE->d_name);

    vkprintf(2, "name class %d\n", name_class);
    if (name_class < 0) {
      continue;
    }
    if (!(name_class & KFS_FILE_SNAPSHOT) && (name_class & KFS_FILE_TEMP)) {
      // .bin.bz.tmp - always ignored, used for pack binlog
      // .bin.tmp - ignored for engines, not ignored for replicator
      // .bin.bz.tmp.repl - ignored for engines, not ignored for replicator
      if (!(flags & KFS_OPEN_REPLICA_OPEN_TMP_REPL_BINLOGS)) {
        continue;
      }
      if ((name_class & KFS_FILE_ZIPPED) && !(name_class & KFS_FILE_REPLICATOR)) {
        continue;
      }
    }

    memcpy(dirname + dirname_len, DE->d_name, len + 1);
    path_len = dirname_len + len;

    if (lstat(dirname, &tmp_stat) < 0) {
      kprintf("warning: unable to stat %s: %m\n", dirname);
      continue;
    }

    if (S_ISLNK(tmp_stat.st_mode)) {
      name_class |= KFS_FILE_SYMLINK;
      if (stat(dirname, &tmp_stat) < 0) {
        kprintf("warning: unable to stat %s: %m\n", dirname);
        continue;
      }
    }

    if (!S_ISREG(tmp_stat.st_mode)) {
      vkprintf(1, "continue :(\n");
      continue;
    }

    if (!R) {
      R = static_cast<kfs_replica *>(calloc(1, sizeof(*R)));
      assert(R);
      R->replica_prefix = strdup(replica_name);
      R->replica_prefix_len = strlen(replica_name);
    }

    FI = static_cast<kfs_file_info *>(calloc(1, sizeof(struct kfs_file_info)));
    assert(FI);

    FI->mtime = tmp_stat.st_mtime;
    FI->refcnt = 1;
    FI->replica = R;
    FI->filename = strdup(dirname);
    FI->filename_len = path_len;
    FI->flags = name_class;
    FI->suffix = FI->filename + dirname_len + filename_len;

    FI->min_log_pos = MMPos[0];
    FI->max_log_pos = MMPos[1];

    FI->kfs_file_type = get_kfs_file_type(name_class);

    FI->file_size = tmp_stat.st_size;
    FI->inode = tmp_stat.st_ino;
    FI->device = tmp_stat.st_dev;

    FI->log_pos = (name_class & 7) ? -1 : 0;

    vkprintf(2, "found file %s, size %lld, name class %d, min pos %lld, max pos %lld\n", dirname, FI->file_size, name_class, FI->min_log_pos, FI->max_log_pos);

    if (FI->kfs_file_type == kfs_binlog) {
      if (binlogs >= MAX_REPLICA_FILES) {
        kprintf("assertion: found at least %d binlogs of %s, it is too much\n", binlogs, replica_name);
      }
      assert(binlogs < MAX_REPLICA_FILES);
      t_binlogs[binlogs++] = FI;
    } else if (FI->kfs_file_type == kfs_enc) {
      if (t_enc) {
        kprintf("assertion: found two enc-files '%s' and '%s'.\n", t_enc->filename, FI->filename);
        exit(1);
      }
      if (flags & KFS_OPEN_REPLICA_FLAG_IGNORE_ENCFILE) {
        /* this feature used in replicator */
        _kfs_file_info_decref(FI);
        FI = NULL;
      } else {
        t_enc = FI;
      }
    } else {
      if (snapshots >= MAX_REPLICA_FILES) {
        kprintf("assertion: found at least %d snapshots of %s, it is too much\n", binlogs, replica_name);
      }
      assert(snapshots < MAX_REPLICA_FILES);
      t_snapshots[snapshots++] = FI;
    }
  }

  closedir(D);

  if (binlogs) {
    int i, m = 0;
    kf_sort(t_binlogs, binlogs);
    for (i = 1; i < binlogs; i++) {
      int need_remove = 0;
      if ((t_binlogs[i]->flags & KFS_FILE_ZIPPED) && t_binlogs[m]->min_log_pos == t_binlogs[i]->min_log_pos) {
        preload_file_info(t_binlogs[m]);
        preload_file_info(t_binlogs[i]);
        if (t_binlogs[i]->log_pos < t_binlogs[m]->log_pos) {
          kprintf("fatal: binlog %s have later name and earlier position than %s\n", t_binlogs[i]->filename, t_binlogs[m]->filename);
          assert(0);
        }
        if (t_binlogs[m]->log_pos == t_binlogs[i]->log_pos) {
          kprintf("warning: skip possible duplicate zipped binlog file '%s', since file '%s' was already found.\n", t_binlogs[i]->filename,
                  t_binlogs[m]->filename);
          need_remove = 1;
        }
      }
      if (need_remove) {
        _kfs_file_info_decref(t_binlogs[i]);
      } else {
        t_binlogs[++m] = t_binlogs[i];
      }
    }
    R->binlog_num = m + 1;
    R->binlogs = static_cast<kfs_file_info **>(malloc(sizeof(void *) * R->binlog_num));
    assert(R->binlogs);
    memcpy(R->binlogs, t_binlogs, sizeof(void *) * R->binlog_num);
  }

  if (snapshots) {
    kf_sort(t_snapshots, snapshots);
    R->snapshot_num = snapshots;
    R->snapshots = static_cast<kfs_file_info **>(malloc(sizeof(void *) * snapshots));
    assert(R->snapshots);
    memcpy(R->snapshots, t_snapshots, sizeof(void *) * snapshots);
  }

  if (t_enc) {
    R->enc = t_enc;
    if (replica_create_crypto_context(R) < 0) {
      kprintf("unable to create encryption context for replica '%s'.\n", replica_name);
      close_replica(R);
      return NULL;
    }
  }

  vkprintf(2, "finished pre-loading KFS replica/slice %s: %d snapshots, %d binlogs, %d encfiles\n", replica_name, snapshots, binlogs, t_enc ? 1 : 0);

  return R;
}

int close_replica(kfs_replica_handle_t R) {
  int i;
  if (!R) {
    return 0;
  }
  for (i = 0; i < R->binlog_num; i++) {
    if (R->binlogs[i]->replica == R) {
      R->binlogs[i]->replica = 0;
    }
    _kfs_file_info_decref(R->binlogs[i]);
  }
  for (i = 0; i < R->snapshot_num; i++) {
    if (R->snapshots[i]->replica == R) {
      R->snapshots[i]->replica = 0;
    }
    _kfs_file_info_decref(R->snapshots[i]);
  }

  if (R->enc) {
    _kfs_file_info_decref(R->enc);
    R->enc = 0;
  }

  free((void *)R->replica_prefix);
  free(R->binlogs);
  free(R->snapshots);

  if (R->ctx_crypto) {
    memset(R->ctx_crypto, 0, sizeof(*R->ctx_crypto));
    free(R->ctx_crypto);
    R->ctx_crypto = NULL;
  }

  free(R);
  return 0;
}

DEFINE_VERBOSITY(kfs_replica);

int update_replica(kfs_replica_handle_t R, int flags) {
  if (!R) {
    return -1;
  }
  kfs_replica_handle_t RN = open_replica(R->replica_prefix, flags);
  if (!RN) {
    return -1;
  }

  /* swap cryptographic fields */
  {
    vk_aes_ctx_t *t = R->ctx_crypto;
    R->ctx_crypto = RN->ctx_crypto;
    RN->ctx_crypto = t;
  }
  {
    struct kfs_file_info *t = R->enc;
    R->enc = RN->enc;
    RN->enc = t;
  }
  if (R->enc) {
    R->enc->replica = R;
  }
  if (RN->enc) {
    RN->enc->replica = RN;
  }

  int i = 0, j = 0;
  struct kfs_file_info *FI, *FJ, **T;

  while (i < RN->binlog_num && j < R->binlog_num) {
    FI = RN->binlogs[i];
    FJ = R->binlogs[j];
    int r = cmp_file_info(FI, FJ);
    if (r < 0) {
      i++;
    } else if (r > 0) {
      j++;
    } else {
      RN->binlogs[i++] = FJ;
      R->binlogs[j++] = FI;
      if (FJ->file_size > FI->file_size) {
        kprintf("Size of %s is decreased: old_size = %lld, new_size = %lld\n", FI->filename, FJ->file_size, FI->file_size);
        assert(FJ->file_size <= FI->file_size);
      }
      if (FJ->file_size < FI->file_size) {
        tvkprintf(kfs_replica, 1, "Updated size of %s: old_size = %lld, new_size = %lld\n", FI->filename, FJ->file_size, FI->file_size);
      }
      FJ->file_size = FI->file_size;
    }
  }

  i = j = 0;
  while (i < RN->snapshot_num && j < R->snapshot_num) {
    FI = RN->snapshots[i];
    FJ = R->snapshots[j];
    int r = cmp_file_info(FI, FJ);
    if (r < 0) {
      i++;
    } else if (r > 0) {
      j++;
    } else {
      RN->snapshots[i++] = FJ;
      R->snapshots[j++] = FI;
      if (FJ->file_size > FI->file_size) {
        kprintf("Strange: I had %s of size %lld (log_pos = %lld), new version with name %s had size %lld (log_pos = %lld)\n", FJ->filename, FJ->file_size,
                FJ->log_pos, FI->filename, FI->file_size, FI->log_pos);
      }
      assert(FJ->file_size <= FI->file_size);
      if (FJ->file_size < FI->file_size) {
        tvkprintf(kfs_replica, 1, "Updated size of %s: old_size = %lld, new_size = %lld\n", FI->filename, FJ->file_size, FI->file_size);
      }
      FJ->file_size = FI->file_size;
    }
  }

  i = R->binlog_num;
  R->binlog_num = RN->binlog_num;
  RN->binlog_num = i;
  T = R->binlogs;
  R->binlogs = RN->binlogs;
  RN->binlogs = T;

  i = R->snapshot_num;
  R->snapshot_num = RN->snapshot_num;
  RN->snapshot_num = i;
  T = R->snapshots;
  R->snapshots = RN->snapshots;
  RN->snapshots = T;

  vkprintf(2, "finished reloading file list for replica %s: %d binlogs, %d snapshots (OLD: %d, %d)\n", R->replica_prefix, R->binlog_num, R->snapshot_num,
           RN->binlog_num, RN->snapshot_num);

  close_replica(RN);

  for (i = 0; i < R->binlog_num; i++) {
    R->binlogs[i]->replica = R;
  }

  for (i = 0; i < R->snapshot_num; i++) {
    R->snapshots[i]->replica = R;
  }

  return 1;
}

/* ---------------- SPECIAL KFS FUNCTIONS FOR ENGINE INITIALISATION ---------------- */

char *engine_replica_name, *engine_snapshot_replica_name;
kfs_replica_handle_t engine_replica, engine_snapshot_replica;

kfs_file_handle_t Snapshot, SnapshotDiff;

struct engine_snapshot_descr engine_snapshot_description;
struct engine_snapshot_descr engine_snapshot_diff_description;

int engine_preload_filelist(const char *main_replica_name, const char *aux_replica_name) {
  int l = strlen(main_replica_name);
  if (!aux_replica_name || !*aux_replica_name || !strcmp(aux_replica_name, ".bin") || !strcmp(aux_replica_name, main_replica_name)
      || (!strncmp(aux_replica_name, main_replica_name, l) && !strcmp(aux_replica_name + l, ".bin"))) {
    engine_snapshot_replica_name = engine_replica_name = strdup(main_replica_name);
  } else {
    int l2 = strlen(aux_replica_name);
    if (l2 > 4 && !strcmp(aux_replica_name + l2 - 4, ".bin")) {
      l2 -= 4;
    }
    engine_snapshot_replica_name = strdup(main_replica_name);
    if (aux_replica_name[0] == '.') {
      engine_replica_name = static_cast<char *>(malloc(l + l2 + 1));
      assert(engine_replica_name);
      memcpy(engine_replica_name, main_replica_name, l);
      memcpy(engine_replica_name + l, aux_replica_name, l2);
      engine_replica_name[l + l2] = 0;
    } else {
      engine_replica_name = static_cast<char *>(malloc(l2 + 1));
      assert(engine_replica_name);
      memcpy(engine_replica_name, aux_replica_name, l2);
      engine_replica_name[l2] = 0;
    }
  }
  assert(engine_replica_name && engine_snapshot_replica_name);

  engine_replica = open_replica(engine_replica_name, 0);
  if (!engine_replica) {
    return -1;
  }
  if (engine_snapshot_replica_name == engine_replica_name) {
    engine_snapshot_replica = engine_replica;
  } else {
    engine_snapshot_replica = open_replica(engine_snapshot_replica_name, KFS_OPEN_REPLICA_FLAG_FORCE);
    if (!engine_snapshot_replica) {
      return 0;
    }
  }

  return 1;
}

void _kfs_file_info_decref(struct kfs_file_info *FI) {
  int refcnt = __sync_sub_and_fetch(&FI->refcnt, 1);
  assert(refcnt >= 0);
  if (refcnt) {
    return;
  }
  free(FI->start);
  free(FI->iv);
  free((char *)FI->filename);
  free(FI);
}
