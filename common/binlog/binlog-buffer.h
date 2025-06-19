// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __KDB_BINLOG_BUFFER_H__
#define __KDB_BINLOG_BUFFER_H__

#include <aio.h>
#include <functional>
#include <limits.h>

#include "common/kfs/kfs.h"
#include "net/net-msg.h"

#include "common/binlog/binlog-buffer-fwdecl.h"
#include "common/binlog/binlog-buffer-replay.h"
#include "common/binlog/binlog-buffer-rotation-points.h"
#include "common/binlog/kdb-binlog-common.h"

#define MAX_LOG_POS (LLONG_MAX / 2)

#define BB_FLAG_STORE_HISTORY 0x01
#define BB_FLAG_LOG_SET_NOW 0x02
#define BB_FLAG_DISABLE_CRC32_WRITE 0x04
#define BB_FLAG_DISABLE_CRC32_CHECK 0x08
#define BB_FLAG_DISABLE_CRC32_EVAL 0x10
#define BB_FLAG_DISABLE_TS_WRITE 0x20
#define BB_FLAG_BINLOG_DISABLED 0x40
#define BB_FLAG_FLUSH_RARELY 0x80

#define BB_FLAG_FAILED_ROTATE_TO 0x10000000
#define BB_FLAG_SOUGHT 0x20000000
// #define BB_FLAG_NEW_DATA_AVAILABLE 0x40000000

/* reader flags */

#define BBR_FLAG_EXIT_AFTER_WRONG_LOGEVENT 0x04000000
#define BBR_FLAG_REACH_ROTATE_TO 0x08000000
#define BBR_FLAG_SILENT_WRONG_LOGEVENT 0x40000000

#define BB_MAX_WRITE_THRESHOLD 0x1000000
#define BB_MAX_LOGEVENT_SIZE 0x1000000

#define BB_SMALL_BUFFER 16384
#define BB_STD_BUFFER 262144

#define BB_LOGEVENT_WANTED_SIZE(size) (-0x40000000 - (size))
#define BB_IS_LOGEVENT_WANTED_SIZE(size) ((size) <= -0x40000000)
#define BB_WANTED_SIZE_TO_ALIGNED_SIZE(r) (((-0x40000000 + 3) - (r)) & -4)

typedef struct bb_rotation_point {
  long long log_pos;
  long long log_slice_start_pos; /* filled with Binlog */
  bb_buffer_t* buffer;
  struct bb_rotation_point *next, *prev;
  kfs_file_handle_t Binlog; /* binlog handle after rotation point */
  struct lev_rotate_to RT;  /* filled by replaying reader */
  int refcnt;
  enum bb_rotation_point_type tp;
} bb_rotation_point_t;

typedef struct bb_reader_functions {
  const char* title;
  void (*init)(bb_reader_t* R);         /* allocates extra field */
  void (*release)(bb_reader_t* R);      /* releases allocated pointers in extra struct */
  int (*work)(bb_reader_t* R, int len); /* process new available data till rotation point */
  void (*seek)(bb_reader_t* R, bb_rotation_point_t* P);
  void (*rotate)(bb_reader_t* R, bb_rotation_point_t* P);
} bb_reader_type_t;

typedef struct bb_writer_functions {
  const char* title;
  void (*init)(bb_writer_t* W);
  void (*release)(bb_writer_t* W); /* releases allocated pointers in extra struct */
  int (*try_read)(bb_writer_t* W, int len);
  void (*seek)(bb_writer_t* W, bb_rotation_point_t* P);
  /* rotate returns -2 if rotation couldn't perform right now */
  int (*rotate)(bb_writer_t* W, bb_rotation_point_t* P);
} bb_writer_type_t;

struct bb_reader {
  bb_buffer_t* buffer;
  bb_reader_type_t* type;
  void* extra;
  bb_rotation_point_t* last_rotation_point;
  long long log_stored_pos;
  long long log_rptr_pos;
  long long log_wptr_pos;
  // long long log_skip_work_pos;
  // long long log_last_processed_rotation_point;
  // int refcnt;
  int flags;
};

typedef struct {
  bb_rotation_point_t* rotation_point;
  long long offset;
} bb_decryption_ctx_t;

struct bb_writer {
  bb_buffer_t* buffer;
  bb_writer_type_t* type;
  bb_rotation_point_t* last_rotation_point;
  void* extra;
  int flags;
  int unsuccessful_rotation_first_ts;
  int unsuccessful_rotation_attempts;
};

struct bb_replay_func;

struct bb_buffer {
  raw_message_t raw;
  bb_rotation_point_t rotation_points; /* rotation points' list */
  char binlog_tag[16];
  long long log_last_rpos; /* first position in raw (replayer will start from here) */
  long long log_last_wpos; /* last position written by writer (in memory buffers, not on disk) */
  long long log_crc32_pos;
  // NB: while reading binlog, this is position of the log event being interpreted;
  //     while writing binlog, this is the last position to have been written to disk, uncommitted log bytes are not included.
  long long log_pos;
  long long log_readto_pos; /* replay_log worked till this position */
  long long log_next_rotate_pos;
  hash_t cur_binlog_file_hash, prev_binlog_file_hash;
  void* extra;
  struct kfs_replica* replica;
  bb_rotation_point_t* rp_cur_binlog;
  struct bb_replay_func* replay_func;
  bb_buffer_t *next, *prev;
  bb_reader_t* reader;
  bb_writer_t* writer;
  int flags;
  int log_first_played_ts;                       // first read timestamp
  int log_first_ts, log_read_until, log_last_ts; // first log timestamp, last read timestamp, last read or written timestamp
  unsigned log_crc32_complement;
  int max_write_threshold;
  int min_write_threshold;
  int err; /* last replay_logevent error code */
};

void bb_buffer_set_writer(bb_buffer_t* B, bb_writer_t* W);
int bb_buffer_set_reader(bb_buffer_t* B, bb_reader_t* R);
void bb_buffer_seek(bb_buffer_t* B, long long log_pos, int timestamp, unsigned log_crc32);
int bb_buffer_replay_log(bb_buffer_t* B, int set_now);
int bb_buffer_work(bb_buffer_t* B);
void bb_buffer_set_flags(bb_buffer_t* B, bb_reader_t* R, int binlog_disabled, int disable_crc32, int disable_ts, int flush_rarely);

void bbr_default_rotate(bb_reader_t* R, bb_rotation_point_t* P);

unsigned bb_buffer_relax_crc32(bb_buffer_t* B, long long pos);

int bb_buffer_writer_try_read(bb_buffer_t* B, int len);

int bb_reader_init(bb_reader_t* R, bb_reader_type_t* type);

int bb_writer_init(bb_writer_t* W, bb_writer_type_t* type);

extern bb_writer_type_t bbw_local_replica_functions;

void bb_buffer_push_data(bb_buffer_t* B, void* data, int size);
void bb_buffer_set_current_binlog(bb_buffer_t* B, bb_rotation_point_t* P);

struct bb_replay_func {
  std::function<int(const struct lev_generic*, int)> func;

  explicit bb_replay_func(std::function<int(const struct lev_generic*, int)> func)
      : func(std::move(func)) {}

  int operator()(const struct lev_generic* E, int size) const {
    return func(E, size);
  }
};

void bb_buffer_init(bb_buffer_t* B, bb_writer_t* W, bb_reader_t* R, std::function<int(const struct lev_generic*, int)> replay_logevent);

void bb_buffer_open_to_replay(bb_buffer_t* buffer, bb_writer_t* writer, bb_reader_t* reader, kfs_replica_t* replica,
                              std::function<int(const struct lev_generic*, int)> replay_func);
#endif
