// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <assert.h>
#include <stdint.h>
#include <sys/cdefs.h>

#include "common/allocators/lockfree-slab.h"
#include "common/kprintf.h"

#define MSG_STD_BUFFER 2048
#define MSG_SMALL_BUFFER 512

DECLARE_VERBOSITY(net_msg);

struct msg_buffer {
  lockfree_slab_cache_tls_t *cache_tls;
  int refcnt;
  char data[0];
};
typedef struct msg_buffer msg_buffer_t;

static inline uint32_t msg_buffer_size(const msg_buffer_t *msg_buffer) {
  return (uint32_t)msg_buffer->cache_tls->extra;
}

void msg_buffers_register_thread();
void preallocate_msg_buffers();

msg_buffer_t *alloc_msg_buffer(int size_hint);

void free_msg_buffer(msg_buffer_t *buffer);

static inline void msg_buffer_inc_ref(msg_buffer_t *buffer) {
  ++buffer->refcnt;
}

static inline int msg_buffer_use_count(const msg_buffer_t *buffer) {
  return buffer->refcnt;
}

static inline void msg_buffer_dec_ref(msg_buffer_t *buffer) {
  if (!--buffer->refcnt) {
    free_msg_buffer(buffer);
  } else {
    assert(buffer->refcnt > 0);
  }
}
