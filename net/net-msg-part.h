// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef KDB_NET_NET_MSG_PART_H
#define KDB_NET_NET_MSG_PART_H

#include <sys/cdefs.h>

#include "net/net-msg-buffers.h"

struct msg_part {
  int refcnt;
  struct msg_part *next;
  msg_buffer_t *buffer;
  int offset; // data offset inside buffer->data
  int len;    // buffer length in bytes
};
typedef struct msg_part msg_part_t;

void init_msg_part();

msg_part_t *new_msg_part(msg_buffer_t *X);
void free_msg_part(msg_part_t *mp);

int rwm_total_msg_parts();

static inline void msg_part_inc_ref(msg_part_t *mp) {
  ++mp->refcnt;
}

static inline int msg_part_dec_ref(msg_part_t *mp) {
  if (!--mp->refcnt) {
    msg_buffer_dec_ref(mp->buffer);
    mp->buffer = NULL;
    mp->next = NULL;
    free_msg_part(mp);
    return 0;
  } else {
    assert(mp->refcnt > 0);
    return mp->refcnt;
  }
}

static inline void msg_part_chain_dec_ref(msg_part_t *mp) {
  while (mp) {
    msg_part_t *mpn = mp->next;
    if (msg_part_dec_ref(mp)) {
      break;
    }
    mp = mpn;
  }
}

static inline int msg_part_use_count(const msg_part_t *mp) {
  return mp->refcnt;
}

#endif // KDB_NET_NET_MSG_PART_H
