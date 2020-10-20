#include "common/server/main-binlog.h"

#include "common/binlog/binlog-buffer.h"

#include "common/stats/provider.h"

bb_buffer_t BinlogBuffer;
bb_reader_t BinlogBufferReader;
bb_writer_t BinlogBufferWriter;

long long log_cur_pos (void) {
  return bb_buffer_log_cur_pos (&BinlogBuffer);
}

long long log_last_pos (void) {
  return log_cur_pos();
}
