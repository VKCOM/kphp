// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

/* for pread */
#if !defined(__APPLE__)
  #define _XOPEN_SOURCE 500
#endif

#include "common/binlog/binlog-buffer.h"

#include <cassert>
#include <climits>
#include <cstring>
#include <utility>
#include <vector>

#include "common/crc32.h"
#include "common/kfs/kfs-binlog.h"
#include "common/kprintf.h"
#include "common/options.h"
#include "common/precise-time.h"

#include "common/binlog/kdb-binlog-common.h"

DEFINE_VERBOSITY(binlog_buffers);

/******************** RWM for binlogs buffers ********************/

void bb_buffer_push_data(bb_buffer_t* B, void* data, int size) {
  rwm_push_data_ext(&B->raw, data, size, 0, BB_SMALL_BUFFER, BB_STD_BUFFER);
}

/******************** binlog buffers ********************/

void bb_buffer_set_writer(bb_buffer_t* B, bb_writer_t* W) {
  assert(B->writer == NULL && W->buffer == NULL);
  B->writer = W;
  W->buffer = B;
}

void bb_buffer_init(bb_buffer_t* B, bb_writer_t* W, bb_reader_t* R, std::function<int(const lev_generic*, int)> replay_func) {
  memset(B, 0, sizeof(*B));
  B->replay_func = new bb_replay_func(std::move(replay_func));
  assert(!B->rotation_points.next && !B->rotation_points.prev);
  B->rotation_points.next = B->rotation_points.prev = &B->rotation_points;
  B->rotation_points.tp = rpt_undef;
  B->rotation_points.log_pos = B->rotation_points.RT.next_log_pos = MAX_LOG_POS;
  B->rotation_points.buffer = B;

  rwm_init(&B->raw, 0);

  bb_buffer_set_writer(B, W);

  B->log_next_rotate_pos = MAX_LOG_POS;
  B->log_crc32_complement = -1;
  B->max_write_threshold = BB_MAX_WRITE_THRESHOLD;
  B->min_write_threshold = 0;

  bb_buffer_set_reader(B, R);
}

void bb_buffer_set_flags(bb_buffer_t* B, int binlog_disabled, int disable_crc32, int disable_ts, int flush_rarely) {
  B->flags &= ~(BB_FLAG_BINLOG_DISABLED | BB_FLAG_DISABLE_CRC32_WRITE | BB_FLAG_DISABLE_CRC32_EVAL | BB_FLAG_DISABLE_TS_WRITE | BB_FLAG_FLUSH_RARELY);
  if (binlog_disabled) {
    B->flags |= BB_FLAG_BINLOG_DISABLED;
  }
  if (disable_crc32 & 1) {
    B->flags |= BB_FLAG_DISABLE_CRC32_WRITE;
  }
  if (disable_crc32 & 2) {
    B->flags |= BB_FLAG_DISABLE_CRC32_CHECK;
  }
  if ((B->flags & (BB_FLAG_DISABLE_CRC32_WRITE | BB_FLAG_DISABLE_CRC32_CHECK)) == (BB_FLAG_DISABLE_CRC32_WRITE | BB_FLAG_DISABLE_CRC32_CHECK)) {
    B->flags |= BB_FLAG_DISABLE_CRC32_EVAL;
  }
  if (disable_ts & 1) {
    B->flags |= BB_FLAG_DISABLE_TS_WRITE;
  }
  if (flush_rarely) {
    B->flags |= BB_FLAG_FLUSH_RARELY;
  }
}

void bb_buffer_seek(bb_buffer_t* B, long long log_pos, int timestamp, unsigned log_crc32) {
  assert(!B->log_last_rpos && !B->log_last_wpos && !B->log_crc32_pos && !B->log_pos);
  B->log_last_rpos = B->log_last_wpos = B->log_crc32_pos = B->log_pos = log_pos;
  B->log_crc32_complement = ~log_crc32;

  B->log_first_ts = B->log_read_until = B->log_last_ts = timestamp;

  bb_rotation_point_t* p = bb_rotation_point_alloc(B, rpt_seek, log_pos);

  if (!(B->writer->flags & BB_FLAG_SOUGHT)) {
    B->writer->type->seek(B->writer, p);
    B->writer->flags |= BB_FLAG_SOUGHT;
    bb_rotation_point_assign(&B->writer->last_rotation_point, p);
  }
}

void bb_buffer_set_current_binlog(bb_buffer_t* B, bb_rotation_point_t* P) {
  assert(P->Binlog);
  bb_rotation_point_assign(&B->rp_cur_binlog, P);
}

int bb_reader_init(bb_reader_t* R, bb_reader_type_t* type) {
  memset(R, 0, sizeof(*R));
  R->type = type;
  if (R->type->init) {
    R->type->init(R);
  }
  return 0;
}

int bb_writer_init(bb_writer_t* W, bb_writer_type_t* type) {
  memset(W, 0, sizeof(*W));
  W->type = type;

  if (W->type->init) {
    W->type->init(W);
  }

  return 0;
}

int bb_buffer_set_reader(bb_buffer_t* B, bb_reader_t* R) {
  assert(B->reader == NULL && R->buffer == NULL);
  R->buffer = B;
  B->reader = R;
  return 0;
}

int bb_buffer_replay_log(bb_buffer_t* B, int set_now) {
  int log_true_now = now;
  B->flags &= ~BB_FLAG_LOG_SET_NOW;
  if (set_now) {
    B->flags |= BB_FLAG_LOG_SET_NOW;
    now = B->log_last_ts;
  }
  int r;
  do {
    r = bb_buffer_work(B);
    vkprintf(3, "%s: bb_buffer_work returns 0x%x.\n", __func__, r);
  } while (r > 0);
  B->log_readto_pos = bb_buffer_log_cur_pos(B);
  if (B->log_readto_pos != B->log_last_wpos) {
    vkprintf(2, "replay binlog '%s' till %lld position, read binlog till %lld position.\n", B->replica->replica_prefix, B->log_last_rpos, B->log_last_wpos);
  }
  now = log_true_now;
  return (B->err < 0) ? B->err : 0;
}

static long long bb_buffer_get_rpos(const bb_buffer_t* B) {
  long long pos = B->log_last_wpos;
  bb_reader_t* p = B->reader;
  long long w = (p->flags & BB_FLAG_STORE_HISTORY) ? p->log_stored_pos : p->log_rptr_pos;
  return w < pos ? w : pos;
}

static bb_rotation_point_t* bb_get_next_rotation_point(bb_buffer_t* B, bb_rotation_point_t* P) {
  P = (P == NULL) ? B->rotation_points.next : P->next;
  assert(P->buffer == B);
  return P;
}

static int noop(void* extra __attribute__((unused)), const void* data __attribute__((unused)), int len __attribute__((unused))) {
  return 0;
}

static int bb_reader_work(bb_reader_t* R, long long pos) {
  vkprintf(4, "%s: update reader till %lld position.\n", __func__, pos);
  assert(R->log_wptr_pos <= pos);
  R->log_wptr_pos = pos;
  if (!(R->flags & BB_FLAG_SOUGHT)) {
    bb_rotation_point_t* P = R->buffer->rotation_points.next;
    assert(P != &R->buffer->rotation_points);
    assert(P->tp == rpt_seek);
    R->log_stored_pos = R->log_rptr_pos = R->log_wptr_pos = P->log_pos;
    R->type->seek(R, P);
    R->flags |= BB_FLAG_SOUGHT;
    bb_rotation_point_assign(&R->last_rotation_point, P);
  }
  const long long old = R->log_rptr_pos;
  for (;;) {
    bb_rotation_point_t* P = bb_get_next_rotation_point(R->buffer, R->last_rotation_point);
    assert(P->tp != rpt_seek);
    long long end = P->log_pos < R->log_wptr_pos ? P->log_pos : R->log_wptr_pos;
    int len = end - R->log_rptr_pos;
    vkprintf(4, "R->log_rptr_pos = %lld, end = %lld, len = %d\n", R->log_rptr_pos, end, len);
    assert(len >= 0);
    if (len > 0) {
      long long start = R->log_rptr_pos;
      int bytes = R->type->work(R, len);
      assert(R->log_rptr_pos <= R->log_wptr_pos);
      if (bytes < 0) {
        break;
      }
      assert(start + bytes == R->log_rptr_pos);
      if (bytes < len) {
        break;
      }
    }
    if (R->log_rptr_pos == P->log_pos) {
      assert(P->log_pos < MAX_LOG_POS);
      assert(P->tp == rpt_slice_end);
      R->type->rotate(R, P);
      bb_rotation_point_assign(&R->last_rotation_point, P);
      continue;
    }
    if (!len) {
      break;
    }
  }
  return R->log_rptr_pos - old;
}

static int process_block_relax_crc32(void* extra, const void* data, int len) {
  bb_buffer_t* B = (bb_buffer_t*)extra;
  B->log_crc32_complement = crc32_partial(data, len, B->log_crc32_complement);
  B->log_crc32_pos += len;
  return 0;
}

unsigned bb_buffer_relax_crc32(bb_buffer_t* B, long long pos) {
  if (!(B->flags & BB_FLAG_DISABLE_CRC32_EVAL) && B->log_crc32_pos < pos) {
    int len = pos - B->log_crc32_pos;
    assert(len > 0);
    rwm_process_from_offset(&B->raw, len, B->log_crc32_pos - B->log_last_rpos, process_block_relax_crc32, B);
  }
  return ~B->log_crc32_complement;
}

int bb_buffer_writer_try_read(bb_buffer_t* B, int len) {
  if (len > B->min_write_threshold) {
    len = B->writer->type->try_read(B->writer, len);
    vkprintf(3, "writer %s reads %d bytes\n", B->writer->type->title, len);
    if (len < 0) {
      return -1;
    }
  } else {
    return 0;
  }
  B->log_last_wpos += len;
  return len;
}

int bb_buffer_work(bb_buffer_t* B) {
  long long pos;
  int r = 0;
  const int len = B->max_write_threshold - bb_buffer_unprocessed_log_bytes(B);
  if (bb_buffer_writer_try_read(B, len) > 0) {
    r |= 1;
  }
  if (bb_reader_work(B->reader, B->log_last_wpos) > 0) {
    r |= 2;
  }
  if (B->raw.total_bytes > 0) {
    /* free unused chunks */
    pos = bb_buffer_get_rpos(B);
    if (B->log_last_rpos < pos) {
      int bytes = pos - B->log_last_rpos;
      bb_buffer_relax_crc32(B, pos);
      rwm_process_and_advance(&B->raw, bytes, noop, NULL);
      B->log_last_rpos = pos;
    }
    /* GC rotation points */
    bb_rotation_point_t *p, *w;
    for (p = B->rotation_points.next; p != &B->rotation_points; p = w) {
      if (p->log_pos >= pos || p->refcnt) {
        break;
      }
      w = p->next;
      bb_rotation_point_free(p);
    }
  }
  return r;
}

void bb_buffer_open_to_replay(bb_buffer_t* buffer, bb_writer_t* writer, bb_reader_t* reader, kfs_replica_t* replica,
                              std::function<int(const lev_generic*, int)> replay_func) {
  bb_writer_init(writer, &bbw_local_replica_functions);
  bb_reader_init(reader, &bbr_replay_functions);
  bb_buffer_init(buffer, writer, reader, std::move(replay_func));
  buffer->replica = replica;
}

/******************** default ********************/

void bbr_default_rotate(bb_reader_t* R, bb_rotation_point_t* P) {
  vkprintf(2, "%s: R=%p (%s), P = {.log_pos=%lld, .tp = %d} \n", __func__, R, R->type->title, P->log_pos, P->tp);
  assert(P->tp == rpt_slice_end);
}
