// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "net/net-msg-part.h"

#include <assert.h>

#include "common/allocators/lockfree-slab.h"
#include "common/parallel/counter.h"

static lockfree_slab_cache_t msg_part_cache;
static __thread lockfree_slab_cache_tls_t msg_part_cache_tls;

PARALLEL_COUNTER(rwm_total_msg_parts);

static void __attribute__((constructor)) msg_part_constructor() {
  lockfree_slab_cache_init(&msg_part_cache, sizeof(msg_part_t));
}

static msg_part_t *alloc_msg_part() {
  PARALLEL_COUNTER_INC(rwm_total_msg_parts);
  return static_cast<msg_part_t *>(lockfree_slab_cache_alloc(&msg_part_cache_tls));
}

msg_part_t *new_msg_part(msg_buffer_t *X) {
  msg_part_t *mp = alloc_msg_part();
  assert(mp);
  mp->refcnt = 1;
  mp->next = 0;
  mp->buffer = X;
  mp->offset = 0;
  mp->len = 0;
  return mp;
}

void init_msg_part() {
  PARALLEL_COUNTER_REGISTER_THREAD(rwm_total_msg_parts);
  lockfree_slab_cache_register_thread(&msg_part_cache, &msg_part_cache_tls);
}

void free_msg_part(msg_part_t *mp) {
  PARALLEL_COUNTER_DEC(rwm_total_msg_parts);
  lockfree_slab_cache_free(&msg_part_cache_tls, mp);
}

int rwm_total_msg_parts() {
  return PARALLEL_COUNTER_READ(rwm_total_msg_parts);
}
