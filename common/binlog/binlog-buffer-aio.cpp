// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <aio.h>
#include <assert.h>
#include <cstdlib>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <unistd.h>

#include "common/kfs/kfs-binlog.h"
#include "common/kprintf.h"
#include "common/precise-time.h"
#include "common/server/signals.h"

#include "common/binlog/binlog-buffer.h"

DECLARE_VERBOSITY(binlog_buffers);

/******************** reading local replica ********************/

static void bbw_local_replica_seek(bb_writer_t *W, bb_rotation_point_t *P) {
  struct kfs_replica *R = W->buffer->replica;
  assert(R);
  assert(P->Binlog == NULL);
  P->Binlog = open_binlog(R, P->log_pos);
  if (!P->Binlog) {
    kprintf("fatal: cannot find binlog for %s, log position %lld\n", R->replica_prefix, P->log_pos);
    assert(0);
  }
  tvkprintf(binlog_buffers, 2, "%s: binlog '%s' was opened from position %lld.\n", __func__, P->Binlog->info->filename, P->log_pos);
  bb_buffer_set_current_binlog(W->buffer, P);
  P->log_slice_start_pos = P->Binlog->info->log_pos;
  if (P->Binlog->info->file_hash) {
    W->buffer->cur_binlog_file_hash = P->Binlog->info->file_hash;
  }
  if (!(P->Binlog->info->flags & KFS_FILE_ZIPPED)) {
    long long off = P->log_pos - P->log_slice_start_pos;
    assert(off >= 0);
    if (off > 0) {
      off += P->Binlog->offset;
      if (lseek(P->Binlog->fd, off, SEEK_SET) != off) {
        kprintf("fatal: cannot lseek binlog '%s' to log position %lld, file offset is %lld. %m\n", P->Binlog->info->filename, P->log_pos, off);
        abort();
      }
    }
  }
}

static int bbw_local_replica_rotate(bb_writer_t *W, bb_rotation_point_t *P) {
  bb_buffer_t *B = W->buffer;
  bb_rotation_point_t *Q = W->last_rotation_point;
  assert(Q);
  assert(Q->log_pos < P->log_pos);
  kfs_file_handle_t Binlog = Q->Binlog;
  P->Binlog = next_binlog(Binlog);

  if (P->Binlog == NULL) {
    update_replica(Binlog->info->replica, 0);
    P->Binlog = next_binlog(Binlog);
  }

  if (P->Binlog == NULL) {
    if (!W->unsuccessful_rotation_attempts) {
      W->unsuccessful_rotation_first_ts = time(NULL);
    }
    W->unsuccessful_rotation_attempts++;
    int t = time(NULL) - W->unsuccessful_rotation_first_ts;
    kprintf("attempt #%d (%d secs): cannot find next binlog file after %s, log position %lld\n", W->unsuccessful_rotation_attempts, t, Binlog->info->filename,
            P->log_pos);
    return (t > 120) ? REPLAY_BINLOG_ERROR : REPLAY_BINLOG_STOP_READING;
  }

  tvkprintf(binlog_buffers, 2, "%s: opened next binlog '%s'(fd=%d) and stored in rotation point %p\n", __func__,
            P->Binlog->info ? P->Binlog->info->filename : "", P->Binlog->fd, P);

  bb_buffer_set_current_binlog(W->buffer, P);
  P->log_slice_start_pos = P->Binlog->info->log_pos;

  if (verbosity > 0) {
    kprintf("switched from binlog file %s to %s at position %lld\n", Binlog->info->filename, P->Binlog->info->filename, P->log_pos);
  }

  assert(B->log_last_ts == P->RT.timestamp);
  assert(bb_buffer_log_cur_pos(B) + 36 == P->RT.next_log_pos);

  W->unsuccessful_rotation_attempts = 0;
  return 0;
}

struct iovec_array {
  struct iovec iov[1024];
  int len;
};

static int local_replica_prepare_iovec_process_block(void *extra, void *data, int len) {
  struct iovec_array *a = (struct iovec_array *)extra;
  if (a->len == sizeof(a->iov) / sizeof(a->iov[0])) {
    return -1;
  }
  struct iovec *v = &a->iov[a->len++];
  v->iov_base = data;
  v->iov_len = len;
  return 0;
}

static int local_replica_decrypt_binlog(void *extra, void *data, int len) {
  bb_decryption_ctx_t *a = (bb_decryption_ctx_t *)extra;
  kfs_buffer_crypt(a->rotation_point->Binlog, data, len, a->offset);
  a->offset += len;
  return 0;
}

int bb_buffer_get_max_alloc_bytes(bb_buffer_t *B) {
  int r = B->max_write_threshold - B->raw.total_bytes;
  if (r < 0) {
    r = 0;
  }
  return r;
}

static int bbw_local_replica_try_read(bb_writer_t *W, int len) {
  bb_buffer_t *B = W->buffer;
  bb_rotation_point_t *P = W->last_rotation_point;
  assert(P);
  assert(P->Binlog);
  assert(P->log_slice_start_pos >= 0);
  int r;
  const long long offset = B->log_last_wpos - P->log_slice_start_pos;
  if (P->Binlog->info->flags & KFS_FILE_ZIPPED) {
    r = 0x1000000;
    auto *a = static_cast<char *>(malloc(r));
    assert(a);
    if (kfs_bz_decode(P->Binlog, offset, a, &r, NULL) < 0) {
      print_backtrace();
      exit(1);
    }
    rwm_trunc(&B->raw, B->log_last_wpos - B->log_last_rpos);
    bb_buffer_push_data(B, a, r);
    free(a);
  } else {
    int alloc_bytes = bb_buffer_get_max_alloc_bytes(B);
    bb_buffer_push_data(B, 0, alloc_bytes);
    static thread_local struct iovec_array ia;
    ia.len = 0;
    rwm_transform_from_offset(&B->raw, len, B->log_last_wpos - B->log_last_rpos, local_replica_prepare_iovec_process_block, &ia);
    ssize_t t = readv(P->Binlog->fd, ia.iov, ia.len);
    if (t < 0) {
      kprintf("fail to read from binlog slice '%s' at position %lld, file offset: %lld. %m\n", P->Binlog->info->filename, B->log_last_wpos,
              offset + P->Binlog->offset);
      assert(0);
    }
    r = t;
    if (verbosity >= 1) {
      int lvl = (r > 0) ? 1 : 3;
      vkprintf(lvl, "read %d bytes from binlog %s\n", r, P->Binlog->info->filename);
    }
    bb_decryption_ctx_t ctx;
    ctx.rotation_point = NULL;
    bb_rotation_point_assign(&ctx.rotation_point, P);
    ctx.offset = offset + P->Binlog->offset;
    rwm_transform_from_offset(&B->raw, r, B->log_last_wpos - B->log_last_rpos, local_replica_decrypt_binlog, &ctx);
    bb_rotation_point_assign(&ctx.rotation_point, NULL);
  }
  return r;
}

bb_writer_type_t bbw_local_replica_functions = [] {
  bb_writer_type_t funcs = bb_writer_type_t();
  funcs.title = "writer(local replica)";
  funcs.try_read = bbw_local_replica_try_read;
  funcs.seek = bbw_local_replica_seek;
  funcs.rotate = bbw_local_replica_rotate;
  return funcs;
}();
