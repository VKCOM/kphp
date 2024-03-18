// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/binlog/binlog-buffer-replay.h"

#include <cstdlib>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common/algorithms/arithmetic.h"
#include "common/container_of.h"
#include "common/md5.h"
#include "common/precise-time.h"
#include "common/wrappers/likely.h"

#include "common/binlog/binlog-buffer.h"
#include "common/binlog/kdb-binlog-common.h"

DECLARE_VERBOSITY(binlog_buffers);

#define UPDATE_OLD_BINLOG_VARS

typedef struct {
  bool need_to_free;
  int attempts;
  int wait_job_num;
} bb_wait_job_cb_t;

typedef struct {
  char buf[BB_MAX_LOGEVENT_SIZE];
  long long log_last_processed_pos; /* not necessary at logevent bound */
  long long replay_redundant_memcpy_bytes;
  int wptr;
  int cur_logevent_size;
  bb_wait_job_cb_t *wait_job_cb;
} bbr_replay_extra_t;

static int bb_writer_rotate(bb_writer_t *W, bb_rotation_point_t *p) {
  if (W->last_rotation_point != p) {
    assert(!W->last_rotation_point || W->last_rotation_point->log_pos <= p->log_pos);
    int r = W->type->rotate(W, p);
    if (r < 0) {
      if (r != -2) {
        kprintf("binlog buffer writer failed to perform rotation at log position %lld.\n", p->log_pos);
      }
      return r;
    }
    bb_rotation_point_assign(&W->last_rotation_point, p);
  }
  return 0;
}

/******************** replay binlog ********************/
void bbr_replay_init(bb_reader_t *R) {
  bbr_replay_extra_t *e = static_cast<bbr_replay_extra_t *>(calloc(sizeof(*e), 1));
  assert(e);
  e->wait_job_cb = static_cast<bb_wait_job_cb_t *>(calloc(sizeof(*e->wait_job_cb), 1));
  R->extra = e;
}

void bbr_replay_release(bb_reader_t *R) {
  bbr_replay_extra_t *e = (bbr_replay_extra_t *)R->extra;
  if (e->wait_job_cb->wait_job_num == 0) {
    free(e->wait_job_cb);
  } else {
    e->wait_job_cb->need_to_free = true;
  }
  e->wait_job_cb = NULL;
}

static void bb_buffer_process_timestamp_event(bb_buffer_t *B, const lev_generic *E) {
  vkprintf(4, "%s: E->a = %d\n", __func__, E->a);
  if (!B->log_first_played_ts) {
    B->log_first_played_ts = E->a;
  }
  if (!B->log_first_ts) {
    B->log_first_ts = E->a;
  }
  if (B->log_read_until > E->a) {
    kprintf("time goes back from %d to %d in log file\n", B->log_read_until, E->a);
  }
  B->log_last_ts = B->log_read_until = E->a;

  if (B->flags & BB_FLAG_LOG_SET_NOW) {
    now = B->log_read_until;
  }
  /*
    if (E->a >= B->log_time_cutoff && !B->log_scan_mode) {
      B->log_cutoff_pos = bb_buffer_log_cur_pos (B);
      B->log_scan_mode = 1;
      if (verbosity) {
        fprintf (stderr, "reached timestamp %d above cutoff %d at binlog position %lld, entering scan mode 1\n",
          E->a, B->log_time_cutoff, B->log_cutoff_pos);
      }
    }
  */
}

static int bb_buffer_process_crc_event(bb_buffer_t *B, struct lev_crc32 *E, int offset) {
  if (!(B->flags & BB_FLAG_DISABLE_CRC32_CHECK)) {
    bb_buffer_relax_crc32(B, B->log_pos);
    if (B->log_crc32_pos != E->pos - offset) {
      kprintf("crc position mismatch: current position %lld, logevent position %lld, logevent timestamp %d, logevent type 0x%08x\n", B->log_crc32_pos, E->pos,
              E->timestamp, E->type);
      return REPLAY_BINLOG_ERROR;
    }
    if (~B->log_crc32_complement != E->crc32) {
      kprintf("crc mismatch at binlog position %lld with timestamp %d: expected %08x, found %08x, logevent type 0x%08x\n", B->log_crc32_pos, E->timestamp,
              E->crc32, ~B->log_crc32_complement, E->type);
      return REPLAY_BINLOG_ERROR;
    }
  }
  bb_buffer_process_timestamp_event(B, (const lev_generic *)E);
  return 0;
}

static long long bb_binlog_fd_get_size(kfs_file_handle_t Binlog, int fd) {
  struct stat st;
  assert(Binlog->info);
  if (fstat(fd, &st) < 0) {
    kprintf("%s: fstat for binlog '%s'(fd:%d) failed. %m\n", __func__, Binlog->info->filename, fd);
    abort();
  }
  return st.st_size;
}

static int bb_binlog_pread(kfs_file_handle_t Binlog, int fd, void *buffer, int count, long long offset) {
  ssize_t r = pread(fd, buffer, count, offset);
  if (r < 0) {
    kprintf("%s: fail pread %d bytes at offset %lld from slice '%s' (fd:%d). %m\n", __func__, count, offset, Binlog->info->filename, fd);
    return -1;
  } else if (r != count) {
    kprintf("%s: fail pread %d bytes at offset %lld from slice '%s' (fd:%d). Read only %d bytes\n", __func__, count, offset, Binlog->info->filename, (int)r,
            fd);
    return -1;
  }
  kfs_buffer_crypt(Binlog, buffer, count, offset);
  return count;
}

static int bb_buffer_process_rotate_to(bb_buffer_t *B, struct lev_rotate_to *RT) {
  int r = bb_buffer_process_crc_event(B, reinterpret_cast<struct lev_crc32 *>(RT), sizeof(*RT));
  if (r < 0) {
    return r;
  }

  assert(RT->cur_log_hash);
  assert(RT->next_log_hash);
  if (!B->cur_binlog_file_hash) {
    B->cur_binlog_file_hash = bb_buffer_calc_binlog_hash(B, NULL);
  }
  assert(RT->cur_log_hash == B->cur_binlog_file_hash);
  bb_rotation_point_t *P = B->rotation_points.prev;
  if (P->log_pos < RT->next_log_pos) {
    P = bb_rotation_point_alloc(B, rpt_slice_end, RT->next_log_pos);
    memcpy(&P->RT, RT, sizeof(*RT));
  } else {
    assert(P->log_pos == RT->next_log_pos);
  }

  r = bb_writer_rotate(B->writer, P);
  if (r < 0) {
    return r;
  }

  B->prev_binlog_file_hash = B->cur_binlog_file_hash;
  B->cur_binlog_file_hash = 0;

  return sizeof(*RT);
}

static int bb_buffer_process_tag(bb_buffer_t *B, struct lev_tag *T) {
  int t;
  for (t = 0; t < 16; t++) {
    if (B->binlog_tag[t]) {
      kprintf("%s: handle LEV_TAG error at position %lld, binlog tag is already set.\n", __func__, bb_buffer_log_cur_pos(B));
      return -1;
    }
  }
  memcpy(B->binlog_tag, T->tag, 16);
  for (t = 0; t < 16; t++) {
    if (B->binlog_tag[t]) {
      break;
    }
  }
  if (t >= 16) {
    kprintf("%s: handle LEV_TAG error at position %lld, zero tag.\n", __func__, bb_buffer_log_cur_pos(B));
    return -1;
  }
  return sizeof(*T);
}

static int bb_buffer_process_rotate_from(bb_buffer_t *B, struct lev_rotate_from *RF) {
  int r = bb_buffer_process_crc_event(B, reinterpret_cast<struct lev_crc32 *>(RF), 0);
  if (r < 0) {
    return r;
  }
  bb_rotation_point_t *P = B->rp_cur_binlog;
  assert(P);
  if (!B->cur_binlog_file_hash) {
    assert(B->prev_binlog_file_hash);
    B->cur_binlog_file_hash = binlog_relax_hash(B->prev_binlog_file_hash, P->RT.next_log_pos, P->RT.crc32);
  }
  assert(bb_buffer_log_cur_pos(B) == RF->cur_log_pos);
  assert(RF->cur_log_pos == P->RT.next_log_pos);
  assert(RF->timestamp == P->RT.timestamp);
  assert(RF->prev_log_hash == P->RT.cur_log_hash);
  assert(RF->cur_log_hash == P->RT.next_log_hash);
  assert(B->prev_binlog_file_hash == P->RT.cur_log_hash);
  assert(B->cur_binlog_file_hash == P->RT.next_log_hash);
  assert(B->prev_binlog_file_hash && B->cur_binlog_file_hash);
  return sizeof(*RF);
}

/******************** bb_buffer_replay_logevent  ********************/

static int bb_process_lev_start(bb_buffer_t *B, const lev_generic *E, int size) {
  int s;
  if (size < 24 || E->b < 0 || E->b > 4096) {
    return REPLAY_BINLOG_NOT_ENOUGH_DATA;
  }
  s = 24 + align4(E->b);
  if (size < s) {
    return BB_LOGEVENT_WANTED_SIZE(s);
  }
  if (!B->replay_func) {
    kprintf("Binlog buffer replay logevent method isn't defined.\n");
    assert(B->replay_func);
  }
  return (*B->replay_func)(E, s);
}

static int bb_process_lev_noop(bb_buffer_t *B __attribute__((unused)), const lev_generic *E __attribute__((unused)), int size) {
  if (size < 4) {
    return REPLAY_BINLOG_NOT_ENOUGH_DATA;
  }
  return 4;
}

static int bb_process_lev_timestamp(bb_buffer_t *B, const lev_generic *E, int size) {
  if (size < 8) {
    return BB_LOGEVENT_WANTED_SIZE(8);
  }
  bb_buffer_process_timestamp_event(B, E);
  return 8;
}

static int bb_process_lev_tag(bb_buffer_t *B, const lev_generic *E, int size) {
  if (size < 20) {
    return BB_LOGEVENT_WANTED_SIZE(20);
  }
  return bb_buffer_process_tag(B, (struct lev_tag *)E);
}

static int bb_process_lev_rotate_from(bb_buffer_t *B, const lev_generic *E, int size) {
  if (size < 36) {
    return BB_LOGEVENT_WANTED_SIZE(36);
  }
  return bb_buffer_process_rotate_from(B, (struct lev_rotate_from *)E);
}

static int bb_process_lev_rotate_to(bb_buffer_t *B, const lev_generic *E, int size) {
  if (size < 36) {
    return BB_LOGEVENT_WANTED_SIZE(36);
  }
  assert(size == 36);
  return bb_buffer_process_rotate_to(B, (struct lev_rotate_to *)E);
}

static int bb_process_lev_crc32(bb_buffer_t *B, const lev_generic *E, int size) {
  if (size < 20) {
    return BB_LOGEVENT_WANTED_SIZE(20);
  }
  int r = bb_buffer_process_crc_event(B, (struct lev_crc32 *)E, 0);
  return (r < 0 ? r : 20);
}

static int bb_process_phash_miss(bb_buffer_t *B, const lev_generic *E, int size) {
  assert(B->replay_func);
  return (*B->replay_func)(E, size);
}

static int bb_buffer_replay_logevent(bb_buffer_t *B, const lev_generic *E, int size) {
  static const struct {
    int (*process)(bb_buffer_t *B, const lev_generic *E, int size);
    unsigned magic;
  } tbl[8] = {{bb_process_lev_timestamp, LEV_TIMESTAMP},
              {bb_process_phash_miss, static_cast<unsigned>(-1)},
              {bb_process_lev_tag, LEV_TAG},
              {bb_process_lev_noop, LEV_NOOP},
              {bb_process_lev_start, LEV_START},
              {bb_process_lev_rotate_from, LEV_ROTATE_FROM},
              {bb_process_lev_crc32, LEV_CRC32},
              {bb_process_lev_rotate_to, LEV_ROTATE_TO}};
  const unsigned idx = (((unsigned int)E->type) * 1611680321u) >> 29;
  if (tbl[idx].magic == E->type) {
    return tbl[idx].process(B, E, size);
  }
  assert(B->replay_func);
  return (*B->replay_func)(E, size);
}

static int process_replay_work(void *extra, const void *data, int len) {
  int r;
  const void *_data = data;
  int l_sum = 0;
  bb_reader_t *R = (bb_reader_t *)extra;
  bbr_replay_extra_t *e = (bbr_replay_extra_t *)R->extra;
  bb_buffer_t *B = R->buffer;
  if (e->wptr > 0) {
    int s = e->cur_logevent_size ? (e->cur_logevent_size - e->wptr) : 64;
    while (len > 0) {
      int l = sizeof(e->buf) - e->wptr;
      if (unlikely(!l)) {
        kprintf("%s: cannot replay logevent at pos %lld, %d bytes buffer was overflown\n", __func__, B->log_pos, (int)sizeof(e->buf));
        assert(l);
      }
      if (likely(l > len)) {
        l = len;
      }
      if (l > s) {
        l = s;
      }
      l &= -4;
      memcpy(e->buf + e->wptr, data, l);
      e->log_last_processed_pos += l;
      e->wptr += l;
      l_sum += l;
      data = (static_cast<const char *>(data) + l);
      len -= l;
      if (e->cur_logevent_size > e->wptr) {
        assert(!len);
        break;
      }
      r = bb_buffer_replay_logevent(B, (const lev_generic *)e->buf, e->wptr);

      if (BB_IS_LOGEVENT_WANTED_SIZE(r)) {
        e->cur_logevent_size = BB_WANTED_SIZE_TO_ALIGNED_SIZE(r);
        s = e->cur_logevent_size - e->wptr;
        continue;
      }

      if (r == REPLAY_BINLOG_NOT_ENOUGH_DATA) {
        s <<= 1;
        continue;
      }

      if (r == REPLAY_BINLOG_WAIT_JOB || r == REPLAY_BINLOG_STOP_READING) {
        e->log_last_processed_pos -= l;
        e->wptr -= l;
        return r;
      }

      if (unlikely(r < 0)) {
        if (!(R->flags & BBR_FLAG_SILENT_WRONG_LOGEVENT)) {
          kprintf("%s: cannot replay logevent at pos %lld, exit code is %d.\n", __func__, R->log_rptr_pos, r);
        }
        if (R->flags & BBR_FLAG_EXIT_AFTER_WRONG_LOGEVENT) {
          kprintf("fatal: ceased reading binlog updates");
          assert(0 && "cannot interpret replicated binlog");
        }
        e->log_last_processed_pos -= l_sum;
        e->wptr -= l_sum;
        return r;
      }
      r = align4(r);
      assert(r <= e->wptr);
      B->log_pos += r;
      R->log_rptr_pos += r;
      l = e->wptr - r;
      assert(l >= 0);
      /* clear buffer and move data pointers backward */
      vkprintf(4, "move %d bytes backward\n", l);
      e->log_last_processed_pos -= l;
      data = (static_cast<const char *>(data) - l);
      assert(data >= _data);
      len += l;
      e->wptr = 0;
      e->replay_redundant_memcpy_bytes += l;
      e->cur_logevent_size = 0;
      break;
    }
  }
  while (len > 0) {
    r = bb_buffer_replay_logevent(B, (const lev_generic *)data, len);
    if (BB_IS_LOGEVENT_WANTED_SIZE(r)) {
      e->cur_logevent_size = BB_WANTED_SIZE_TO_ALIGNED_SIZE(r);
      assert(len < e->cur_logevent_size);
      assert(len <= sizeof(e->buf));
      memcpy(e->buf, data, len);
      e->log_last_processed_pos += len;
      e->wptr = len;
      return 0;
    }
    if (r == REPLAY_BINLOG_NOT_ENOUGH_DATA) {
      assert(len <= sizeof(e->buf));
      memcpy(e->buf, data, len);
      e->log_last_processed_pos += len;
      e->wptr = len;
      return 0;
    }
    if (r == REPLAY_BINLOG_WAIT_JOB || r == REPLAY_BINLOG_STOP_READING) {
      return r;
    }
    if (unlikely(r < 0)) {
      if (!(R->flags & BBR_FLAG_SILENT_WRONG_LOGEVENT)) {
        kprintf("%s: cannot replay logevent at pos %lld, exit code is %d.\n", __func__, R->log_rptr_pos, r);
      }
      if (R->flags & BBR_FLAG_EXIT_AFTER_WRONG_LOGEVENT) {
        assert(r >= 0);
      }
      return r;
    }
    assert(r <= len);
    r = align4(r);
    e->log_last_processed_pos += r;
    B->log_pos += r;
    R->log_rptr_pos += r;
    data = (static_cast<const char *>(data) + r);
    len -= r;
  }
  return 0;
}

static int bbr_replay_work(bb_reader_t *R, int len) {
  vkprintf(4, "%s: R=%p, len = %d\n", __func__, R, len);
  bbr_replay_extra_t *e = static_cast<bbr_replay_extra_t *>(R->extra);
  assert(e);
  if (e->wait_job_cb->wait_job_num) {
    return 0;
  }
  const long long pos = R->log_rptr_pos;
  assert(pos <= e->log_last_processed_pos);
  len = R->log_wptr_pos - e->log_last_processed_pos;
  R->buffer->err = rwm_process_from_offset(&R->buffer->raw, len, e->log_last_processed_pos - R->buffer->log_last_rpos, process_replay_work, R);

  if (R->log_rptr_pos != pos) {
    e->wait_job_cb->attempts = 0;
  }

  if (R->buffer->err == REPLAY_BINLOG_WAIT_JOB) {
    if (++e->wait_job_cb->attempts >= 10) {
      kprintf("Too many wait_aio in row: binlog %s log_pos %lld\n", R->buffer->replica->replica_prefix, pos);
      assert(0 && "binlog replay: too many wait_aio in row");
    }
    tvkprintf(binlog_buffers, 3, "REPLAY_BINLOG_WAIT_JOB at pos %lld (wait_job_num %d attempts %d)\n", pos, e->wait_job_cb->wait_job_num,
              e->wait_job_cb->attempts);
    R->buffer->err = 0;
  } else {
    e->wait_job_cb->attempts = 0;
  }
  return R->log_rptr_pos - pos;
}

static void bbr_replay_seek(bb_reader_t *R, bb_rotation_point_t *P) {
  vkprintf(2, "%s: R=%p, P = {.log_pos=%lld, .tp = %d} \n", __func__, R, P->log_pos, P->tp);
  assert(P->tp == rpt_seek);
  bbr_replay_extra_t *e = static_cast<bbr_replay_extra_t *>(R->extra);
  assert(e);
  e->wptr = 0;
  e->log_last_processed_pos = P->log_pos;
}

long long bb_buffer_log_cur_pos(bb_buffer_t *B) {
  return B->log_pos;
}

long long bb_buffer_unprocessed_log_bytes(bb_buffer_t *B) {
  return B->log_last_wpos - B->log_last_rpos;
}

hash_t binlog_calc_hash(kfs_file_handle_t F, struct lev_rotate_to *next_rotate_to) {
  assert(F);
  const struct kfs_file_info *FI = F->info;
  assert(FI);
  assert(!FI->log_pos);
  if (FI->khdr) {
    return FI->khdr->file_id_hash;
  }

  if (FI->flags & KFS_FILE_ZIPPED) {
    const kfs_binlog_zip_header_t *H = kfs_get_binlog_zip_header(FI);
    assert(*(int *)H->first36_bytes == LEV_START);
    return H->file_hash;
  }

  int fd = open(FI->filename, O_RDONLY);
  if (fd < 0) {
    kprintf("%s: cannot open '%s' in read-only mode. %m\n", __func__, FI->filename);
    abort();
  }

  const long long fsize = bb_binlog_fd_get_size(F, fd);
  vkprintf(3, "%s: binlog slice '%s' size is %lld bytes.\n", __func__, FI->filename, fsize);
  const int bufsize = next_rotate_to ? sizeof(struct lev_rotate_to) : 0;
  const long long expected_size = fsize + bufsize;
  if (expected_size < offsetof(struct lev_start, str) + sizeof(struct lev_rotate_to)) {
    kprintf("%s: not enough data (%lld bytes) for contain LEV_START and LEV_ROTATE_TO logevents\n", __func__, expected_size);
    close(fd);
    abort();
  }

  unsigned char *buffer = static_cast<unsigned char *>(malloc(32768));
  assert(buffer);
  void *logbuf = next_rotate_to;

  if (expected_size <= 32768) {
    int r = bb_binlog_pread(F, fd, buffer, fsize, 0LL);
    assert(r == fsize);
    if (bufsize > 0) {
      assert(logbuf);
      memcpy(buffer + r, logbuf, bufsize);
    }
  } else if (fsize >= 16384) {
    int r1 = bb_binlog_pread(F, fd, buffer, 16384, 0LL);
    assert(r1 == 16384);
    int toread = 16384 - bufsize;
    assert(toread > 0);
    /* TODO: binlog_cyclic_mode */
    int r2 = bb_binlog_pread(F, fd, buffer + 16384, toread, fsize - toread);
    assert(r2 == toread);
    if (bufsize > 0) {
      assert(logbuf);
      memcpy(buffer + 16384 + r2, logbuf, bufsize);
    }
  } else {
    assert(0 && "impossible situation since bufsize is equal 0 or 36 in current implementation");
    int r1 = bb_binlog_pread(F, fd, buffer, fsize, 0LL);
    assert(r1 == fsize);
    memcpy(buffer + r1, logbuf, 16384 - fsize);
    memcpy(buffer + 16384, static_cast<char *>(logbuf) + bufsize - 16384, 16384);
  }

  assert(!close(fd));
  assert(*(int *)buffer == LEV_START);
  int totsize = expected_size < 32768 ? expected_size : 32768;
  memset(buffer + totsize - 16, 0, 16);

  static unsigned char md[16];
  md5(buffer, totsize, md);
  free(buffer);
  return *(hash_t *)md;
}

hash_t bb_buffer_calc_binlog_hash(bb_buffer_t *B, struct lev_rotate_to *next_rotate_to) {
  bb_rotation_point_t *p = B->rotation_points.next;
  assert(p);
  assert(p != &B->rotation_points);
  return binlog_calc_hash(p->Binlog, next_rotate_to);
}

hash_t binlog_relax_hash(hash_t prev_hash, long long pos, unsigned log_crc32) {
  struct {
    hash_t prev_hash;
    long long stpos;
    unsigned crc32;
  } cbuff;
  unsigned char MDBUF[16];
  cbuff.prev_hash = prev_hash;
  cbuff.stpos = pos;
  cbuff.crc32 = log_crc32;
  md5((unsigned char *)&cbuff, 20, MDBUF);
  return *((hash_t *)MDBUF);
}

bb_reader_type_t bbr_replay_functions = {.title = "reader(replay)",
                                         .init = bbr_replay_init,
                                         .release = bbr_replay_release,
                                         .work = bbr_replay_work,
                                         .seek = bbr_replay_seek,
                                         .rotate = bbr_default_rotate};
