// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/server/main-binlog.h"

#include "common/binlog/binlog-buffer.h"

#include "common/stats/provider.h"

bb_buffer_t BinlogBuffer;
bb_reader_t BinlogBufferReader;
bb_writer_t BinlogBufferWriter;

long long log_cur_pos() {
  return bb_buffer_log_cur_pos(&BinlogBuffer);
}

long long log_last_pos() {
  return log_cur_pos();
}
