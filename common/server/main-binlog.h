// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <stdbool.h>
#include <sys/cdefs.h>

#include "common/binlog/binlog-buffer-fwdecl.h"

extern bb_buffer_t BinlogBuffer;
extern bb_reader_t BinlogBufferReader;
extern bb_writer_t BinlogBufferWriter;

typedef unsigned int lev_type_t;

/**
 * NB: while reading binlog, this is position of the log event being interpreted;
 * while writing binlog, this is the last position to have been written to disk, uncommitted log bytes are not included.
 * log_cur_pos could be used in replay binlog stage, in all other situations you probably want to use log_last_pos
 */
long long log_cur_pos();
/**
 * @return calls log_cur_pos
 */
long long log_last_pos();
