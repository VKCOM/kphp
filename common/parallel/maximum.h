// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef KDB_COMMON_PARALLEL_MAXIMUM_H
#define KDB_COMMON_PARALLEL_MAXIMUM_H

#include <sys/cdefs.h>

#include <stdint.h>

#include <pthread.h>

#include "common/parallel/thread-id.h"

struct parallel_maximum_tls {
  uint64_t counter;
  uint64_t counter_max;
};
typedef struct parallel_maximum_tls parallel_maximum_tls_t;

struct parallel_maximum {
  uint64_t global_count;
  uint64_t global_reserve;
  uint64_t global_maximum;
  uint64_t thread_max;
  pthread_mutex_t mtx;
};
typedef struct parallel_maximum parallel_maximum_t;

void parallel_maximum_register_thread(parallel_maximum_t* maximum, parallel_maximum_tls_t* tls);
void parallel_maximum_unregister_thread(parallel_maximum_t* maximum, parallel_maximum_tls_t* tls);
void parallel_maximum_init(parallel_maximum_t* maximum, uint64_t counter_max);
void parallel_maximum_add_slow_path(parallel_maximum_t* maximum, parallel_maximum_tls_t* tls, uint64_t value);
void parallel_maximum_sub_slow_path(parallel_maximum_t* maximum, parallel_maximum_tls_t* tls, uint64_t value);

static inline void parallel_maximum_add(parallel_maximum_t* maximum, parallel_maximum_tls_t* tls, uint64_t value) {
  if (tls->counter_max - tls->counter >= value) {
    tls->counter += value;
    return;
  }

  return parallel_maximum_add_slow_path(maximum, tls, value);
}

static inline void parallel_maximum_sub(parallel_maximum_t* maximum, parallel_maximum_tls_t* tls, uint64_t value) {
  if (tls->counter >= value) {
    tls->counter -= value;
    return;
  }

  return parallel_maximum_sub_slow_path(maximum, tls, value);
}

static inline uint64_t parallel_maximum_read(const parallel_maximum_t* maximum) {
  return maximum->global_maximum;
}

static inline uint64_t parallel_maximum_read_current(const parallel_maximum_t* maximum) {
  return maximum->global_count + maximum->global_reserve;
}

#define PARALLEL_MAXIMUM(name)                                                                                                                                 \
  static __thread parallel_maximum_tls_t parallel_maximum_##name##_tls;                                                                                        \
  static parallel_maximum_t parallel_maximum_##name;

#define PARALLEL_MAXIMUM_REGISTER_THREAD(name) parallel_maximum_register_thread(&parallel_maximum_##name, &parallel_maximum_##name##_tls)
#define PARALLEL_MAXIMUM_UNREGISTER_THREAD(name) parallel_maximum_unregister_thread(&parallel_maximum_##name, &parallel_maximum_##name##_tls)
#define PARALLEL_MAXIMUM_INIT(name, thread_max) parallel_maximum_init(&parallel_maximum_##name, thread_max)

#define PARALLEL_MAXIMUM_ADD(name, value) parallel_maximum_add(&parallel_maximum_##name, &parallel_maximum_##name##_tls, value)
#define PARALLEL_MAXIMUM_SUB(name, value) parallel_maximum_sub(&parallel_maximum_##name, &parallel_maximum_##name##_tls, value)
#define PARALLEL_MAXIMUM_READ(name) parallel_maximum_read(&parallel_maximum_##name)
#define PARALLEL_MAXIMUM_READ_CURRENT(name) parallel_maximum_read_current(&parallel_maximum_##name)

#endif // KDB_COMMON_PARALLEL_MAXIMUM_H
