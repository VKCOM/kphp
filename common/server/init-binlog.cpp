// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/server/init-binlog.h"

#include "common/binlog/binlog-buffer.h"

#include "common/kprintf.h"
#include "common/options.h"
#include "common/precise-time.h"
#include "common/server/engine-settings.h"
#include "common/server/main-binlog.h"
#include "common/tl/constants/common.h"
#include "common/tl/constants/engine.h"
#include "common/tl/parse.h"

static int replay_logevent_wrapper(const struct lev_generic* E, int size) {
  engine_settings_t* engine_settings = get_engine_settings();
  auto do_replay = [&]() {
    assert(engine_settings->replay_logevent);
    int result = engine_settings->replay_logevent(const_cast<lev_generic*>(E), size);
    if (result == REPLAY_BINLOG_ERROR) {
      kprintf("error while reading log event type %08x at position %lld\n", E->type, log_cur_pos());
    }
    return result;
  };
  switch (E->type) {
  case LEV_NOOP:
  case LEV_CRC32:
  case LEV_TIMESTAMP:
  case LEV_ROTATE_FROM:
  case LEV_ROTATE_TO:
  case LEV_TAG:
  case LEV_SET_PERSISTENT_CONFIG_VALUE:
  case LEV_SET_PERSISTENT_CONFIG_ARRAY:
  case LEV_CBINLOG_END:
    return default_replay_logevent(E, size);
  case LEV_START: {
    struct lev_start* EE = (struct lev_start*)E;
    if (size < offsetof(struct lev_start, str)) {
      return REPLAY_BINLOG_NOT_ENOUGH_DATA;
    }
    if (EE->extra_bytes < 0 || EE->extra_bytes > 4096) {
      return -1;
    }
    int s = (int)(offsetof(struct lev_start, str) + EE->extra_bytes);
    if (size < s) {
      return REPLAY_BINLOG_NOT_ENOUGH_DATA;
    }
    if (engine_settings) {
      if (engine_settings->on_lev_start != NULL) {
        engine_settings->on_lev_start(EE);
      } else {
        return do_replay();
      }
    }
    return s;
  }
  default: {
    return do_replay();
  }
  }
}

static void engine_read_binlog_new() {
  binlog_load_time = -get_utime_monotonic();
  bb_buffer_open_to_replay(&BinlogBuffer, &BinlogBufferWriter, &BinlogBufferReader, engine_replica, replay_logevent);

  bb_buffer_set_flags(&BinlogBuffer, 1, 0, 0, 0);

  bb_buffer_seek(&BinlogBuffer, jump_log_pos, jump_log_ts, jump_log_crc32);

  bb_buffer_replay_log(&BinlogBuffer, 1);
  vkprintf(2, "%s: binlog was replayed till %lld position.\n", __func__, BinlogBuffer.log_readto_pos);
  vkprintf(3, "after replaying now is equal to %d, log_last_ts = %d\n", now, BinlogBuffer.log_last_ts);

  binlog_load_time += get_utime_monotonic();
}

void engine_default_read_binlog() {
  engine_settings_t* engine_settings = get_engine_settings();
  assert(engine_settings);
  if (engine_settings->replay_logevent) {
    replay_logevent = replay_logevent_wrapper;
  }
  assert(engine_replica);

  engine_read_binlog_new();
}

void binlog_try_read_events() {
  if (BinlogBuffer.replay_func) {
    bb_buffer_replay_log(&BinlogBuffer, true);
  }
}
