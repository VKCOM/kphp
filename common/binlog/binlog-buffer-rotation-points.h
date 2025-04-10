// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef ENGINE_BINLOG_BUFFER_ROTATION_POINTS_H
#define ENGINE_BINLOG_BUFFER_ROTATION_POINTS_H

#include <sys/cdefs.h>

struct bb_buffer;

enum bb_rotation_point_type {
  rpt_undef = 0,
  rpt_seek = 1,
  rpt_slice_end = 2,
};

struct bb_rotation_point* bb_rotation_point_alloc(struct bb_buffer* B, enum bb_rotation_point_type tp, long long log_pos);
void bb_rotation_point_assign(struct bb_rotation_point** lhs, struct bb_rotation_point* rhs);

void bb_rotation_point_close_binlog(struct bb_rotation_point* p);
void bb_rotation_point_free(struct bb_rotation_point* p);

#endif // ENGINE_BINLOG_BUFFER_ROTATION_POINTS_H
