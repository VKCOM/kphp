// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef KDB_COMMON_PARALLEL_LIMIT_COUNTER_H
#define KDB_COMMON_PARALLEL_LIMIT_COUNTER_H

#include <sys/cdefs.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <pthread.h>

#include "common/parallel/thread-id.h"

struct parallel_limit_counter_tls {
  uint64_t counter;
  uint64_t counter_max;
};
typedef struct parallel_limit_counter_tls parallel_limit_counter_tls_t;

struct parallel_limit_counter {
  uint64_t global_limit;
  uint64_t global_count;
  uint64_t global_reserve;
  uint64_t thread_max;
  parallel_limit_counter_tls_t* ptrs[NR_THREADS];
  pthread_mutex_t mtx;
};
typedef struct parallel_limit_counter parallel_limit_counter_t;

void parallel_limit_counter_register_thread(parallel_limit_counter_t* counter, parallel_limit_counter_tls_t* tls);
void parallel_limit_counter_init(parallel_limit_counter_t* counter, uint64_t global_limit, uint64_t thread_max);
void parallel_limit_counter_unregister_thread(parallel_limit_counter_t* counter, parallel_limit_counter_tls_t* tls);
uint64_t parallel_limit_counter_read(parallel_limit_counter_t* counter);
bool parallel_limit_counter_add_slow_path(parallel_limit_counter_t* counter, parallel_limit_counter_tls_t* tls, uint64_t value);
bool parallel_limit_counter_sub_slow_path(parallel_limit_counter_t* counter, parallel_limit_counter_tls_t* tls, uint64_t value);

static inline bool parallel_limit_counter_add(parallel_limit_counter_t* counter, parallel_limit_counter_tls_t* tls, uint64_t value) {
  if (tls->counter_max - tls->counter >= value) {
    tls->counter += value;
    return true;
  }

  return parallel_limit_counter_add_slow_path(counter, tls, value);
}

static inline bool parallel_limit_counter_inc(parallel_limit_counter_t* counter, parallel_limit_counter_tls_t* tls) {
  return parallel_limit_counter_add(counter, tls, 1);
}

static inline bool parallel_limit_counter_sub(parallel_limit_counter_t* counter, parallel_limit_counter_tls_t* tls, uint64_t value) {
  if (tls->counter >= value) {
    tls->counter -= value;
    return true;
  }

  return parallel_limit_counter_sub_slow_path(counter, tls, value);
}

static inline bool parallel_limit_counter_dec(parallel_limit_counter_t* counter, parallel_limit_counter_tls_t* tls) {
  return parallel_limit_counter_sub(counter, tls, 1);
}

static inline uint64_t parallel_limit_counter_read_approx(const parallel_limit_counter_t* counter) {
  return counter->global_count + counter->global_reserve;
}

#define PARALLEL_LIMIT_COUNTER(name)                                                                                                                           \
  static __thread parallel_limit_counter_tls_t parallel_limit_counter_##name##_tls;                                                                            \
  static parallel_limit_counter_t parallel_limit_counter_##name;

#define PARALLEL_LIMIT_COUNTER_EXTERN(name)                                                                                                                    \
  __thread parallel_limit_counter_tls_t parallel_limit_counter_##name##_tls;                                                                                   \
  parallel_limit_counter_t parallel_limit_counter_##name;

#define DECLARE_PARALLEL_LIMIT_COUNTER(name)                                                                                                                   \
  extern __thread parallel_limit_counter_tls_t parallel_limit_counter_##name##_tls;                                                                            \
  extern parallel_limit_counter_t parallel_limit_counter_##name;

#define PARALLEL_LIMIT_COUNTER_REGISTER_THREAD(name)                                                                                                           \
  parallel_limit_counter_register_thread(&parallel_limit_counter_##name, &parallel_limit_counter_##name##_tls)
#define PARALLEL_LIMIT_COUNTER_INIT(name, global_limit, thread_max) parallel_limit_counter_init(&parallel_limit_counter_##name, global_limit, thread_max)
#define PARALLEL_LIMIT_COUNTER_UNREGISTER_THREAD(name)                                                                                                         \
  parallel_limit_counter_unregister_thread(&parallel_limit_counter_##name, &parallel_limit_counter_##name##_tls)
#define PARALLEL_LIMIT_COUNTER_ADD(name, value) parallel_limit_counter_add(&parallel_limit_counter_##name, &parallel_limit_counter_##name##_tls, value)
#define PARALLEL_LIMIT_COUNTER_INC(name) parallel_limit_counter_inc(&parallel_limit_counter_##name, &parallel_limit_counter_##name##_tls)
#define PARALLEL_LIMIT_COUNTER_SUB(name, value) parallel_limit_counter_sub(&parallel_limit_counter_##name, &parallel_limit_counter_##name##_tls, value)
#define PARALLEL_LIMIT_COUNTER_DEC(name) parallel_limit_counter_dec(&parallel_limit_counter_##name, &parallel_limit_counter_##name##_tls)
#define PARALLEL_LIMIT_COUNTER_READ_APPROX(name) parallel_limit_counter_read_approx(&parallel_limit_counter_##name)
#define PARALLEL_LIMIT_COUNTER_READ(name) parallel_limit_counter_read(&parallel_limit_counter_##name)

#endif // KDB_COMMON_PARALLEL_LIMIT_COUNTER_H
