// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef KDB_COMMON_PARALLEL_COUNTER_H
#define KDB_COMMON_PARALLEL_COUNTER_H

#include <sys/cdefs.h>

#include <stddef.h>
#include <stdint.h>

#include <pthread.h>

#include "common/parallel/thread-id.h"

struct parallel_counter_tls {
  uint64_t counter;
};
typedef struct parallel_counter_tls parallel_counter_tls_t;

struct parallel_counter {
  uint64_t final;
  parallel_counter_tls_t *ptrs[NR_THREADS];
  pthread_mutex_t mtx;
};
typedef struct parallel_counter parallel_counter_t;

static inline void parallel_counter_inc(parallel_counter_tls_t *tls) {
  ++tls->counter;
}

static inline void parallel_counter_add(parallel_counter_tls_t *tls, uint64_t value) {
  tls->counter += value;
}

static inline void parallel_counter_dec(parallel_counter_tls_t *tls) {
  --tls->counter;
}

static inline void parallel_counter_sub(parallel_counter_tls_t *tls, uint64_t value) {
  tls->counter -= value;
}

void parallel_counter_register_thread(parallel_counter_t *counter, parallel_counter_tls_t *tls);
void parallel_counter_init(parallel_counter_t *counter);
void parallel_counter_unregister_thread(parallel_counter_t *counter, parallel_counter_tls_t *tls);

uint64_t parallel_counter_read(parallel_counter_t *counter);

#define PARALLEL_COUNTER(name)                                                                                                                                 \
  static __thread parallel_counter_tls_t parallel_counter_##name##_tls;                                                                                        \
  static parallel_counter_t parallel_counter_##name;

#define PARALLEL_COUNTER_EXTERN(name)                                                                                                                          \
  __thread parallel_counter_tls_t parallel_counter_##name##_tls;                                                                                               \
  parallel_counter_t parallel_counter_##name;

#define DECLARE_PARALLEL_COUNTER(name)                                                                                                                         \
  extern __thread parallel_counter_tls_t parallel_counter_##name##_tls;                                                                                        \
  extern parallel_counter_t parallel_counter_##name;

#define PARALLEL_COUNTER_REGISTER_THREAD(name) parallel_counter_register_thread(&parallel_counter_##name, &parallel_counter_##name##_tls)
#define PARALLEL_COUNTER_INIT(name) parallel_counter_init(&parallel_counter_##name)
#define PARALLEL_COUNTER_UNREGISTER_THREAD(name) parallel_counter_unregister_thread(&parallel_counter_##name, &parallel_counter_##name##_tls)

#define PARALLEL_COUNTER_INC(name) parallel_counter_inc(&parallel_counter_##name##_tls)
#define PARALLEL_COUNTER_DEC(name) parallel_counter_dec(&parallel_counter_##name##_tls)
#define PARALLEL_COUNTER_ADD(name, value) parallel_counter_add(&parallel_counter_##name##_tls, value)
#define PARALLEL_COUNTER_SUB(name, value) parallel_counter_sub(&parallel_counter_##name##_tls, value)
#define PARALLEL_COUNTER_READ(name) parallel_counter_read(&parallel_counter_##name)

#endif // KDB_COMMON_PARALLEL_COUNTER_H
