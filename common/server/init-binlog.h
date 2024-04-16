// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <stdbool.h>

#include "common/binlog/kdb-binlog-common.h"

/* Differences between replay_logevent_optimized_for_binlog_buffers and old replay_logevent:
   1) replay_logevent_optimized_for_binlog_buffers never called for logevents LEV_TIMESTAMP, LEV_CRC32, LEV_TAG, LEV_ROTATE_TO, LEV_ROTATE_FROM. So these constants could be eliminated from the switch.
   2) replay_logevent_optimized_for_binlog_buffers could return BB_LOGEVENT_WANTED_SIZE(logevent_size)
      when old replay_logevent returns -2. This is usefull for engines which allocates large logevents.
      This optimization reduces number of calls of replay_logevent and memory copying bytes
      when logevent isn't stored in one binlog buffer chunk.
*/

void engine_default_read_binlog();
void binlog_try_read_events();
