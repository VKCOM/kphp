// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <stddef.h>

#include "common/kfs/kfs-common.h"

#include "common/binlog/kdb-binlog-common.h"

struct bb_buffer;
struct lev_rotate_to;
struct bb_reader_functions;

long long bb_buffer_log_cur_pos(struct bb_buffer* B);
long long bb_buffer_unprocessed_log_bytes(struct bb_buffer* B);

hash_t binlog_calc_hash(kfs_file_handle_t F, struct lev_rotate_to* next_rotate_to);
hash_t bb_buffer_calc_binlog_hash(struct bb_buffer* B, struct lev_rotate_to* next_rotate_to);
hash_t binlog_relax_hash(hash_t prev_hash, long long pos, unsigned log_crc32);

extern struct bb_reader_functions bbr_replay_functions;
