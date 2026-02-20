// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/binlog/binlog-buffer-rotation-points.h"

#include <unistd.h>

#include "common/kfs/kfs-binlog.h"
#include "common/kprintf.h"

#include "common/binlog/binlog-buffer.h"

DECLARE_VERBOSITY(binlog_buffers);

/******************** rotation points ********************/

void bb_rotation_point_close_binlog(bb_rotation_point_t* p) {
  if (p->Binlog) {
    tvkprintf(binlog_buffers, 2, "%s: close binlog '%s'(fd=%d) from rotation point %p\n", __func__, p->Binlog->info ? p->Binlog->info->filename : "",
              p->Binlog->fd, p);
    if (fsync(p->Binlog->fd) < 0) {
      kprintf("error syncing binlog: %m\n");
    }
    close_binlog(p->Binlog, true);
    p->Binlog = NULL;
  }
}

void bb_rotation_point_free(bb_rotation_point_t* p) {
  assert(p->tp != rpt_undef);
  vkprintf(3, "%s: tp = %d, log_pos = %lld\n", __func__, p->tp, p->log_pos);
  p->prev->next = p->next;
  p->next->prev = p->prev;
  bb_rotation_point_close_binlog(p);
  p->buffer = NULL;
  p->next = p->prev = NULL;
  free(p);
}

static void bb_rotation_point_decref(bb_rotation_point_t* p) {
  if (p == NULL) {
    return;
  }
  int refcnt = __sync_sub_and_fetch(&p->refcnt, 1);
  if (refcnt) {
    assert(refcnt > 0);
  } else {
    assert(p->buffer);
    if (p->buffer->log_last_rpos > p->log_pos) {
      bb_rotation_point_free(p);
    }
  }
}

static const char* rp_str(bb_rotation_point_t* P) {
  switch (P->tp) {
  case rpt_undef:
    return "rpt_undef";
  case rpt_seek:
    return "rpt_seek";
  case rpt_slice_end:
    return "rpt_slice_end";
  }
  assert(0);

  return NULL;
}

static void bb_buffer_insert_rotation_point(bb_buffer_t* B, bb_rotation_point_t* P) {
  vkprintf(2, "insert %s rotation point for %lld position for binlog '%s'\n", rp_str(P), P->log_pos, B->replica->replica_prefix);
  P->buffer = B;
  bb_rotation_point_t *u = B->rotation_points.prev, *v = &B->rotation_points;
  assert(u == v || u->log_pos < P->log_pos);
  u->next = P;
  P->prev = u;
  v->prev = P;
  P->next = v;
}

bb_rotation_point_t* bb_rotation_point_alloc(bb_buffer_t* B, enum bb_rotation_point_type tp, long long log_pos) {
  auto* p = static_cast<bb_rotation_point_t*>(calloc(1, sizeof(bb_rotation_point_t)));
  assert(p);
  p->tp = tp;
  p->log_pos = log_pos;
  p->log_slice_start_pos = -1;
  bb_buffer_insert_rotation_point(B, p);
  return p;
}

void bb_rotation_point_assign(bb_rotation_point_t** lhs, bb_rotation_point_t* rhs) {
  if (*lhs == rhs) {
    return;
  }
  bb_rotation_point_decref(*lhs);
  *lhs = rhs;
  if (rhs) {
    __sync_add_and_fetch(&rhs->refcnt, 1);
  }
}
