// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#if !defined(__APPLE__)
#define _XOPEN_SOURCE 500
#endif

#include "common/binlog/kdb-binlog-common.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "common/algorithms/arithmetic.h"
#include "common/crc32.h"
#include "common/kprintf.h"
#include "common/md5.h"
#include "common/options.h"
#include "common/server/engine-settings.h"
#include "common/server/main-binlog.h"
#include "common/stats/provider.h"
#include "common/kfs/kfs.h"

int log_split_min, log_split_max, log_split_mod;


int lev_start_logrec_size (const lev_start_t *E, int size) {
  if (size < 24 || E->extra_bytes < 0 || E->extra_bytes > 4096) {
    return REPLAY_BINLOG_NOT_ENOUGH_DATA;
  }
  int s = 24 + align4(E->extra_bytes);
  if (size < s) {
    return REPLAY_BINLOG_NOT_ENOUGH_DATA;
  }
  return s;
}

int default_replay_logevent (const struct lev_generic *E, int size) {
  switch (E->type) {
  case LEV_START: {
    int s = lev_start_logrec_size((lev_start_t *)E, size);
    if (s < 0) {
      return s;
    }
    if (replay_logevent != default_replay_logevent) {
      return replay_logevent(E, s);
    }
    return s;
  }
  case LEV_SET_PERSISTENT_CONFIG_VALUE: {
    struct lev_set_persistent_config_value *EE = (struct lev_set_persistent_config_value *) E;
    if (size < sizeof(*EE) + EE->name_len) {
      return REPLAY_BINLOG_NOT_ENOUGH_DATA;
    }
    return sizeof(*EE) + EE->name_len;
  }
  case LEV_SET_PERSISTENT_CONFIG_ARRAY: {
    struct lev_set_persistent_config_array *EE = (struct lev_set_persistent_config_array *) E;
    const int expected_size = sizeof(*EE) + EE->array_len * sizeof(int) + EE->name_len;
    if (size < expected_size) {
      return REPLAY_BINLOG_NOT_ENOUGH_DATA;
    }
    return expected_size;
  }
  case LEV_CBINLOG_END:
    return REPLAY_BINLOG_STOP_READING;
  }
  kprintf ("unknown logevent type %08x read at position %lld before START log event\n", E->type, log_cur_pos());
  return -1;
}

replay_logevent_vector_t replay_logevent = default_replay_logevent;

double binlog_load_time;
long long jump_log_pos;
int jump_log_ts;
unsigned jump_log_crc32;
