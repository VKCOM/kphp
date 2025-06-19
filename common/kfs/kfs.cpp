// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/kfs/kfs.h"

#include "zlib/zlib.h"
#include "zstd/zstd.h"
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "common/binlog/kdb-binlog-common.h"
#include "common/binlog/snapshot-shifts.h"
#include "common/crc32.h"
#include "common/crc32c.h"
#include "common/kprintf.h"
#include "common/options.h"
#include "common/precise-time.h"
#include "common/sha1.h"
#include "common/wrappers/likely.h"
#include "common/wrappers/pathname.h"

#include "common/kfs/kfs-internal.h"
#include "common/kfs/kfs-layout.h"
#include "common/kfs/kfs-snapshot.h"

#define START_BUFFER_SIZE 12288

int lock_whole_file(int fd, int mode) {
  static struct flock L;
  L.l_type = mode;
  L.l_whence = SEEK_SET;
  L.l_start = 0;
  L.l_len = 0;
  if (fcntl(fd, F_SETLK, &L) < 0) {
    kprintf("cannot lock file %d with mode %s: %m\n", fd,
            mode == F_RDLCK   ? "F_RDLCK"
            : mode == F_WRLCK ? "F_WRLCK"
            : mode == F_UNLCK ? "F_UNLCK"
                              : "(unknown)");
    return -1;
  }
  return 1;
}

/*
 (\.[0-9]{6,})?(\.tmp|\.bin|\.bin\.bz)?
 0 = binlog, 1 = snapshot;
 +2 = .tmp
 +4 = not-first (position present)
 +16 = zipped
 +32 = .enc
 +64 = temporary file created by replicator
 +128 = DIFF-snapshot
*/

int kfs_classify_suffix(const char* suffix, long long* MMPos, const char* filename) {
  int c = 0;
  MMPos[0] = MMPos[1] = 0;
  if (!*suffix) {
    kprintf("[warning] no suffix in kfs file '%s'. Did you add extra .000000.bin or .bin to arg?\n", filename);
    return 1;
  }
  if (*suffix != '.') {
    return -1;
  }
  int i = 1;
  while (suffix[i] >= '0' && suffix[i] <= '9') {
    i++;
  }
  if (i > 1) {
    if (i < 7) {
      return -1;
    }
    int power = (suffix[1] - '0') * 10 + (suffix[2] - '0');
    if (power > 18) {
      return -1;
    }

    long long base = strtoll(suffix + 3, 0, 10);

    int cnt = power - (i - 7);
    if (cnt < 0) {
      return -1;
    }

    if (power > 0 && suffix[3] == '0') {
      return -1;
    }

    MMPos[0] = base;
    MMPos[1] = base;

    int j;
    for (j = 0; j < cnt; j++) {
      MMPos[0] *= 10;
      MMPos[1] = MMPos[1] * 10 + 9;
    }

    suffix += i;
    if (!*suffix) {
      return (KFS_FILE_SNAPSHOT | KFS_FILE_NOT_FIRST);
    }
    if (*suffix != '.') {
      return -1;
    }
    c = KFS_FILE_NOT_FIRST;
  } else {
    MMPos[0] = 0;
    MMPos[1] = (1LL << 62);
  }

  switch (suffix[1]) {
  case 'b':
    if (!strcmp(suffix, ".bin")) {
      return c;
    }
    if (!strcmp(suffix, ".bin.bz")) {
      return c | KFS_FILE_ZIPPED;
    }
    if (!strcmp(suffix, ".bin.bz.tmp")) {
      return (c | KFS_FILE_ZIPPED | KFS_FILE_TEMP);
    }
    return strcmp(suffix, ".bin.bz.tmp.repl") ? -1 : (c | KFS_FILE_ZIPPED | KFS_FILE_TEMP | KFS_FILE_REPLICATOR);
  case 'd':
    if (!strcmp(suffix, ".diff")) {
      return c | KFS_FILE_SNAPSHOT | KFS_FILE_SNAPSHOT_DIFF;
    }
    if (!strcmp(suffix, ".diff.tmp")) {
      return c | KFS_FILE_SNAPSHOT | KFS_FILE_SNAPSHOT_DIFF | KFS_FILE_TEMP;
    }
    return -1;
  case 'e':
    return (c || strcmp(suffix, ".enc")) ? -1 : KFS_FILE_ENCRYPTED;
  case 't':
    return strcmp(suffix, ".tmp") ? -1 : (c | KFS_FILE_SNAPSHOT | KFS_FILE_TEMP);
  case 's':
    if (!strcmp(suffix, ".sz")) {
      return (KFS_FILE_SNAPSHOT | KFS_FILE_NOT_FIRST);
    }
    return strcmp(suffix, ".sz.tmp") ? -1 : (c | KFS_FILE_SNAPSHOT | KFS_FILE_TEMP);
  }
  return -1;
}

int kfs_file_compute_initialization_vector(struct kfs_file_info* FI) {
  vkprintf(3, "computing IV for slice '%s'\n", FI->filename);
  kfs_replica_handle_t R = FI->replica;
  if (R == NULL || R->ctx_crypto == NULL || FI->iv != NULL || FI->kfs_file_type == kfs_enc) {
    return 0;
  }
  // keyring is not suported anymore
  return -1;
}

static int check_kfs_header_basic(const struct kfs_file_header* H) {
  assert(H->magic == KFS_MAGIC);
  if (compute_crc32(H, offsetof(struct kfs_file_header, header_crc32)) != H->header_crc32) {
    return -1;
  }
  if (H->kfs_version != KFS_V01) {
    return -1;
  }
  return 0;
}

int kfs_bz_get_chunks_no(long long orig_file_size) {
  return (orig_file_size + (KFS_BINLOG_ZIP_CHUNK_SIZE - 1)) >> KFS_BINLOG_ZIP_CHUNK_SIZE_EXP;
}

kfs_binlog_zip_header_t* kfs_get_binlog_zip_header(const struct kfs_file_info* FI) {
  assert(FI->start);
  assert(FI->kfs_headers >= 0 && FI->kfs_headers < 2);
  return (kfs_binlog_zip_header_t*)(FI->start + sizeof(struct kfs_file_header) * FI->kfs_headers);
}

int kfs_bz_compute_header_size(long long orig_file_size) {
  int chunks = kfs_bz_get_chunks_no(orig_file_size);
  return sizeof(kfs_binlog_zip_header_t) + 8 * chunks + 4;
}

static int process_first36_bytes(struct kfs_file_info* FI, int fd, int r, const struct lev_start* E) {
  switch (E->type) {
  case LEV_START:
    assert(r >= sizeof(struct lev_start) - 4);
    FI->log_pos = 0;
    break;
  case LEV_ROTATE_FROM:
    if (r < sizeof(struct lev_rotate_from)) {
      kprintf("error reading %d bytes from %s (read only %d bytes)\n", (int)sizeof(struct lev_rotate_from), FI->filename, r);
      return -2;
    }
    FI->log_pos = ((struct lev_rotate_from*)E)->cur_log_pos;
    if (!FI->file_hash) {
      FI->file_hash = ((struct lev_rotate_from*)E)->cur_log_hash;
    } else if (FI->file_hash != ((struct lev_rotate_from*)E)->cur_log_hash) {
      kprintf("warning: binlog file %s has different hash in file info structure (%016llX) and continue record (%016llX)\n", FI->filename, FI->file_hash,
              ((struct lev_rotate_from*)E)->cur_log_hash);
      assert(close(fd) >= 0);
      return -2;
    }
    if (FI->khdr && FI->khdr->file_id_hash != ((struct lev_rotate_from*)E)->cur_log_hash) {
      kprintf("warning: binlog file %s has different hash in header (%016llX) and continue record (%016llX)\n", FI->filename, FI->khdr->file_id_hash,
              ((struct lev_rotate_from*)E)->cur_log_hash);
      assert(close(fd) >= 0);
      return -2;
    }
    if (FI->khdr && FI->khdr->prev_log_hash != ((struct lev_rotate_from*)E)->prev_log_hash) {
      kprintf("warning: binlog file %s has different hash of previous binlog in header (%016llX) and continue record (%016llX)\n", FI->filename,
              FI->khdr->prev_log_hash, ((struct lev_rotate_from*)E)->prev_log_hash);
      assert(close(fd) >= 0);
      return -2;
    }
    if (FI->khdr && FI->khdr->log_pos_crc32 != ((struct lev_rotate_from*)E)->crc32) {
      kprintf("warning: binlog file %s has different crc32 in header (%08X) and continue record (%08X)\n", FI->filename, FI->khdr->log_pos_crc32,
              ((struct lev_rotate_from*)E)->crc32);
      assert(close(fd) >= 0);
      return -2;
    }
    if (FI->khdr && FI->khdr->prev_log_time != ((struct lev_rotate_from*)E)->timestamp) {
      kprintf("warning: binlog file %s has different timestamp in header (%d) and continue record (%d)\n", FI->filename, FI->khdr->prev_log_time,
              ((struct lev_rotate_from*)E)->timestamp);
      assert(close(fd) >= 0);
      return -2;
    }
    break;
  default:
    kprintf("warning: binlog file %s begins with wrong entry type %08x\n", FI->filename, E->type);
    assert(close(fd) >= 0);
    return -2;
  }
  return 0;
}

static int process_binlog_zip_header(struct kfs_file_info* FI, int fd, const kfs_binlog_zip_header_t* H) {
  int r = FI->preloaded_bytes;
  if (r < sizeof(kfs_binlog_zip_header_t)) {
    kprintf("Size of %s is too small: %d, should be atleast %d\n", FI->filename, r, (int)sizeof(kfs_binlog_zip_header_t));
  }
  assert(r >= sizeof(kfs_binlog_zip_header_t));
  if (H->orig_file_size < 36 || H->orig_file_size > KFS_BINLOG_ZIP_MAX_FILESIZE) {
    kprintf("wrong binlog zip header in the file '%s', illegal orig_file_size (%lld)\n", FI->filename, H->orig_file_size);
    assert(close(fd) >= 0);
    return -1;
  }
  int header_size = kfs_bz_compute_header_size(H->orig_file_size) + sizeof(struct kfs_file_header) * FI->kfs_headers;
  if (r < header_size) {
    FI->start = static_cast<char*>(realloc(FI->start, header_size));
    assert(FI->start);
    H = kfs_get_binlog_zip_header(FI);
    int sz = header_size - r, w = read(fd, FI->start + r, sz);
    if (w < 0) {
      kprintf("fail to read %d bytes of chunk table for the file '%s'. %m\n", sz, FI->filename);
      assert(close(fd) >= 0);
      return -1;
    }
    if (w != sz) {
      kprintf("read %d of expected %d bytes of header for the file '%s'.\n", w, sz, FI->filename);
      assert(close(fd) >= 0);
      return -1;
    }
    if (FI->iv) {
      kfs_replica_handle_t R = FI->replica;
      assert(R && R->ctx_crypto);
      assert(r >= sizeof(struct kfs_file_header) * FI->kfs_headers);
      R->ctx_crypto->ctr_crypt(R->ctx_crypto, (unsigned char*)FI->start + r, (unsigned char*)FI->start + r, sz, FI->iv, r);
    }
    r = FI->preloaded_bytes = header_size;
  }

  int chunks = kfs_bz_get_chunks_no(H->orig_file_size);
  unsigned int* header_crc32 = (unsigned int*)&H->chunk_offset[chunks];

  if (compute_crc32(H, header_size - 4 - sizeof(struct kfs_file_header) * FI->kfs_headers) != *header_crc32) {
    kprintf("corrupted zipped binlog header in the file '%s', CRC32 failed.\n", FI->filename);
    assert(close(fd) >= 0);
    return -1;
  }
  return process_first36_bytes(FI, fd, 36, (struct lev_start*)H->first36_bytes);
}

int kfs_count_headers(const struct kfs_file_header* kfs_Hdr, int r, int strict_check) {
  int headers = 0;
  if (r >= 4096 && kfs_Hdr[0].magic == KFS_MAGIC) {
    if (check_kfs_header_basic(kfs_Hdr) < 0) {
      if (!strict_check) {
        return 0;
      }
      kprintf("bad kfs header #0\n");
      return -1;
    }
    headers++;
    if (r >= 8192 && kfs_Hdr[1].magic == KFS_MAGIC) {
      if (check_kfs_header_basic(kfs_Hdr + 1) < 0) {
        if (!strict_check) {
          return 1;
        }
        kprintf("bad kfs header #1\n");
        return -1;
      }
      headers++;
      if (kfs_Hdr[1].header_seq_num == kfs_Hdr[0].header_seq_num) {
        assert(!memcmp(kfs_Hdr + 1, kfs_Hdr, 4096));
      }
    }
  }
  return headers;
}

int preload_file_info_ex(struct kfs_file_info* FI) {
  if (!FI->start) {
    if (kfs_file_compute_initialization_vector(FI) < 0) {
      kprintf("Can't compute AES initialization vector for %s file %s.\n", FI->flags & KFS_FILE_SNAPSHOT ? "snapshot" : "binlog", FI->filename);
      return -2;
    }
    int fd = open(FI->filename, O_RDONLY);
    if (fd < 0) {
      kprintf("Cannot open %s file %s: %m\n", FI->flags & KFS_FILE_SNAPSHOT ? "snapshot" : "binlog", FI->filename);
      return -2;
    }
    FI->start = static_cast<char*>(malloc(START_BUFFER_SIZE));
    assert(FI->start);
    int r = read(fd, FI->start, START_BUFFER_SIZE);
    if (r < 0) {
      kprintf("Cannot read %s file %s: %m\n", FI->flags & KFS_FILE_SNAPSHOT ? "snapshot" : "binlog", FI->filename);
      assert(close(fd) >= 0);
      free(FI->start);
      FI->start = 0;
      return -2;
    }
    FI->preloaded_bytes = r;
    struct kfs_file_header* kfs_Hdr = (struct kfs_file_header*)FI->start;
    int headers = kfs_count_headers(kfs_Hdr, r, FI->iv == NULL);
    if (headers < 0) {
      assert(close(fd) >= 0);
      free(FI->start);
      FI->start = 0;
      return -2;
    }

    FI->khdr = headers ? kfs_Hdr : 0;
    if (headers > 1 && kfs_Hdr[1].header_seq_num > kfs_Hdr[0].header_seq_num) {
      FI->khdr++;
    }

    assert(!headers || FI->khdr->data_size + headers * sizeof(struct kfs_file_header) == FI->khdr->raw_size);
    assert(!headers || FI->khdr->kfs_file_type == FI->kfs_file_type);

    FI->kfs_headers = headers;

    if (FI->iv) {
      kfs_replica_handle_t R = FI->replica;
      assert(R && R->ctx_crypto);
      const int o = KFS_HEADER_SIZE * headers;
      R->ctx_crypto->ctr_crypt(R->ctx_crypto, (unsigned char*)FI->start + o, (unsigned char*)FI->start + o, r - o, FI->iv, o);
    }

    if (FI->kfs_file_type == kfs_binlog) {

      struct lev_start* E = (struct lev_start*)(kfs_Hdr + headers);

      r -= sizeof(struct kfs_file_header) * headers;

      switch (E->type) {
      case LEV_START:
      case LEV_ROTATE_FROM:
        if (FI->flags & KFS_FILE_ZIPPED) {
          kprintf("error: zipped binlog file '%s' starts from LEV_START or LEV_ROTATE_FROM.\n", FI->filename);
          assert(close(fd) >= 0);
          return -2;
        }
        if (process_first36_bytes(FI, fd, r, E) < 0) {
          return -2;
        }
        break;
      case KFS_BINLOG_ZIP_MAGIC:
        if (!(FI->flags & KFS_FILE_ZIPPED)) {
          kprintf("error: not zipped binlog file '%s' contains KFS_BINLOG_ZIP_MAGIC\n", FI->filename);
          assert(close(fd) >= 0);
          return -2;
        }
        if (process_binlog_zip_header(FI, fd, (kfs_binlog_zip_header_t*)E) < 0) {
          return -2;
        }
        break;
      default:
        // TODO: maybe send it to assertion chat?
        if (!FI->file_size) {
          kprintf("error: binlog file %s is empty, somebody is doing something nasty\n", FI->filename);
        } else {
          kprintf("warning: binlog file %s begins with wrong entry type %08x\n", FI->filename, E->type);
        }
        assert(close(fd) >= 0);
        return -2;
      }

      if (FI->khdr && FI->khdr->log_pos != FI->log_pos) {
        kprintf("warning: binlog file %s has different starting position in header (%lld) and starting record (%lld)\n", FI->filename, FI->khdr->log_pos,
                FI->log_pos);
        assert(close(fd) >= 0);
        return -2;
      }

      if (FI->log_pos < FI->min_log_pos || FI->log_pos > FI->max_log_pos) {
        kprintf("warning: binlog file %s starts from position %lld (should be in %lld..%lld)\n", FI->filename, FI->log_pos, FI->min_log_pos, FI->max_log_pos);
        assert(close(fd) >= 0);
        return -2;
      }

      vkprintf(2, "preloaded binlog file info for %s (%lld bytes, %d headers), covering %lld..%lld, name corresponds to %lld..%lld\n", FI->filename,
               FI->file_size, headers, FI->log_pos, FI->log_pos + FI->file_size - 4096 * headers, FI->min_log_pos, FI->max_log_pos);
    } else if (FI->kfs_file_type == kfs_snapshot) {
      FI->snapshot_log_pos = get_snapshot_log_pos(FI);
      if (FI->snapshot_log_pos != -1) {
        if (FI->snapshot_log_pos < FI->min_log_pos || FI->snapshot_log_pos > FI->max_log_pos) {
          kprintf("warning: snapshot file %s starts from position %lld (should be in %lld..%lld)\n", FI->filename, FI->log_pos, FI->min_log_pos,
                  FI->max_log_pos);
          assert(close(fd) >= 0);
          return -2;
        }
      }
    }

    if (FI->khdr && FI->replica) {
      if (!FI->khdr->replica_id_hash) {
        kprintf("warning: binlog file %s has zero replica_id_hash, skipping\n", FI->filename);
        assert(close(fd) >= 0);
        return -2;
      }
      if (!FI->replica->replica_id_hash) {
        FI->replica->replica_id_hash = FI->khdr->replica_id_hash;
      } else if (FI->replica->replica_id_hash != FI->khdr->replica_id_hash) {
        kprintf("warning: binlog file %s has incorrect replica_id_hash %016llx != %016llx\n", FI->filename, FI->khdr->replica_id_hash,
                FI->replica->replica_id_hash);
      }
    }

    assert(lseek(fd, sizeof(struct kfs_file_header) * headers, SEEK_SET) == sizeof(struct kfs_file_header) * headers);

    return fd;
  } else {
    return -1;
  }
}

bool preload_file_info(struct kfs_file_info* FI) {
  int fd = preload_file_info_ex(FI);
  if (fd >= 0) {
    assert(close(fd) >= 0);
    return true;
  }
  // -1 stands for "already preloaded"; it's ok as long as we don't care about fd
  return fd == -1;
}

int kfs_close_file(kfs_file_handle_t F, bool close_handle) {
  if (!F) {
    return 0;
  }
  if (F->fd >= 0) {
    if (close_handle) {
      int r = close(F->fd);
      if (r < 0) {
        kprintf("close(F->fd %d) returns %d: %m\n", F->fd, r);
        assert(r >= 0);
      }
    }
    F->fd = -1;
  }
  if (F->info) {
    _kfs_file_info_decref(F->info);
    F->info = 0;
  }
  free(F);
  return 0;
}

#define N_BUFFERS_LIMIT 256
static unsigned char* bz_decode_buffers[N_BUFFERS_LIMIT];
static int bz_decode_buffers_size;

static unsigned char* alloc_buffer(size_t size) {
  int i = __sync_fetch_and_add(&bz_decode_buffers_size, 1);
  assert(i < N_BUFFERS_LIMIT);
  return bz_decode_buffers[i] = static_cast<unsigned char*>(malloc(size));
}

__attribute__((destructor)) static void free_decode_buffers() {
  for (int i = 0; i < bz_decode_buffers_size; i++) {
    free(bz_decode_buffers[i]);
  }
}

int kfs_bz_decode(const struct kfs_file* F, long long off, void* dst, int* dest_len, int* disk_bytes_read) {
  assert(F->offset == 0 || F->offset == 4096 || F->offset == 8192);
  vkprintf(3, "F.offset = %lld, off = %lld, dst = %p, *dest_len = %d\n", F->offset, off, dst, *dest_len);
  if (disk_bytes_read) {
    *disk_bytes_read = 0;
  }
  const struct kfs_file_info* FI = F->info;
  const kfs_binlog_zip_header_t* H = kfs_get_binlog_zip_header(FI);
  assert(H);
  const int chunks = kfs_bz_get_chunks_no(H->orig_file_size), fd = F->fd;

  if (off < 0) {
    kprintf("negative file offset '%lld', file '%s'.\n", off, FI->filename);
    return -1;
  }

  if (off > H->orig_file_size) {
    kprintf("file offset '%lld' is greater than original file size '%lld', file '%s'.\n", off, H->orig_file_size, FI->filename);
    return -1;
  }

  if (off == H->orig_file_size) {
    *dest_len = 0;
    return 0;
  }

  const long long chunk_no_ll = off >> KFS_BINLOG_ZIP_CHUNK_SIZE_EXP;
  if (chunk_no_ll >= chunks) {
    kprintf("chunk_no (%lld) >= chunks (%d)\n", chunk_no_ll, chunks);
    return -1;
  }

  int chunk_no = (int)chunk_no_ll;
  int avail_out = *dest_len, written_bytes = 0;
  int o = off & (KFS_BINLOG_ZIP_CHUNK_SIZE - 1);

  while (chunk_no < chunks) {
    const long long chunk_size =
        (chunk_no < chunks - 1 ? H->chunk_offset[chunk_no + 1] : FI->file_size - sizeof(struct kfs_file_header) * FI->kfs_headers) - H->chunk_offset[chunk_no];
    const long long chunk_offset = H->chunk_offset[chunk_no] + F->offset;
    if (chunk_size <= 0) {
      kprintf("not positive chunk size (%lld), broken header(?), file: %s\n, chunk: %d, chunk_offset: %lld\n", chunk_size, FI->filename, chunk_no,
              chunk_offset);
      return -1;
    }
    if (chunk_size > KFS_BINLOG_ZIP_MAX_ENCODED_CHUNK_SIZE) {
      kprintf("chunk size (%lld) > KFS_BINLOG_ZIP_MAX_ENCODED_CHUNK_SIZE (%d), file: %s, chunk: %d, chunk_offset: %lld\n", chunk_size,
              KFS_BINLOG_ZIP_MAX_ENCODED_CHUNK_SIZE, FI->filename, chunk_no, chunk_offset);
      return -1;
    }

    int expected_output_bytes = chunk_no == chunks - 1 ? (H->orig_file_size & (KFS_BINLOG_ZIP_CHUNK_SIZE - 1)) : KFS_BINLOG_ZIP_CHUNK_SIZE;

    if (avail_out < expected_output_bytes) {
      break;
    }

    vkprintf(3, "chunk_no: %d, chunks: %d, chunk_size: %lld, chunk_off: %lld\n", chunk_no, chunks, chunk_size, chunk_offset);
    if (lseek(fd, chunk_offset, SEEK_SET) != chunk_offset) {
      kprintf("lseek to chunk (%d), offset %lld of file '%s' failed. %m\n", chunk_no, chunk_offset, FI->filename);
      return -1;
    }
    static __thread unsigned char* src;
    if (src == NULL) {
      src = alloc_buffer(KFS_BINLOG_ZIP_MAX_ENCODED_CHUNK_SIZE);
    }
    ssize_t r = read(fd, src, chunk_size);
    if (r < 0) {
      kprintf("read chunk (%d), offset %lld of file '%s' failed. %m\n", chunk_no, chunk_offset, FI->filename);
      return -1;
    }
    if (disk_bytes_read) {
      *disk_bytes_read += r;
    }
    if (r != chunk_size) {
      kprintf("read only %lld of expected %lld bytes, chunk (%d), offset %lld, file '%s'.\n", (long long)r, chunk_size, chunk_no, chunk_offset, FI->filename);
      return -1;
    }
    if (FI->iv) {
      kfs_replica_handle_t R = FI->replica;
      assert(R && R->ctx_crypto);
      R->ctx_crypto->ctr_crypt(R->ctx_crypto, src, src, chunk_size, FI->iv, chunk_offset);
    }
    vkprintf(2, "read %lld bytes from the file '%s', chunk: %d.\n", (long long)r, FI->filename, chunk_no);

    int m = expected_output_bytes;
    switch (H->format & 15) {
    case kfs_bzf_zlib: {
      uLongf destLen = m;
      int res = uncompress(static_cast<unsigned char*>(dst), &destLen, src, chunk_size);
      if (res != Z_OK) {
        kprintf("uncompress returns error code %d, chunk %d, offset %lld, file '%s'.\n", res, chunk_no, chunk_offset, FI->filename);
        return -1;
      }
      m = (int)destLen;
      break;
    }
    default:
      kprintf("Unimplemented format '%d' in the file '%s'.\n", H->format & 15, FI->filename);
      return -1;
    }
    if (expected_output_bytes != m) {
      kprintf("expected chunks size is %d, but decoded bytes number is %d, file: '%s', chunk_no: %d, chunk_offset: %lld\n", expected_output_bytes, m,
              FI->filename, chunk_no, chunk_offset);
      return -1;
    }

    int w = -1;
    if (o > 0) {
      w = m - o;
      if (w <= 0) {
        break;
      }
      memmove(dst, static_cast<char*>(dst) + o, w);
    } else {
      w = m;
    }
    assert(w >= 0);
    dst = static_cast<char*>(dst) + w;
    avail_out -= w;
    written_bytes += w;
    o = 0;
    chunk_no++;
  }
  *dest_len = written_bytes;
  return 0;
}

void kfs_buffer_crypt(kfs_file_handle_t F, void* buff, long long size, long long off) {
  assert(off >= 0);
  if (F && F->info && F->info->iv) {
    /* don't crypt KFS headers */
    assert(F->offset == sizeof(struct kfs_file_header) * F->info->kfs_headers);
    long long skip_bytes = F->offset - off;
    if (skip_bytes > 0) {
      vkprintf(3, "skip %lld bytes from %d unencrypted KFS headers\n", skip_bytes, F->info->kfs_headers);
      size -= skip_bytes;
      if (size <= 0) {
        return;
      }
      off = F->offset;
      buff = static_cast<char*>(buff) + skip_bytes;
    }
    _buffer_crypt(F->info->replica, (unsigned char*)buff, size, F->info->iv, off);
  }
}

long long kfs_read_file(kfs_file_handle_t F, void* buff, long long size) {
  long long off = -1;
  if (F->info && F->info->iv) {
    off = lseek(F->fd, 0, SEEK_CUR);
    if (off < 0) {
      kprintf("cannot obtain offset of the file '%s'. %m\n", F->info->filename);
      exit(1);
    }
  }
  long long read_bytes = 0;
  while (size > 0) {
    long long r = read(F->fd, buff, size);
    if (r < 0) {
      kprintf("error reading file '%s'. %m\n", F->info->filename);
      exit(1);
    }
    if (r != size) {
      vkprintf(1, "read %lld out of %lld during single read function call while reading file '%s'\n", r, size, F->info->filename);
    }
    if (!r) {
      return read_bytes;
    }
    if (off >= 0) {
      kfs_buffer_crypt(F, buff, r, off);
      off += r;
    }
    read_bytes += r;
    buff = static_cast<char*>(buff) + r;
    size -= r;
  }
  return read_bytes;
}
