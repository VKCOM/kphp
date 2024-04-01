// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/parallel/counter.h"

void parallel_counter_init(parallel_counter_t *counter) {
  counter->final = 0;
  for (int i = 0; i < NR_THREADS; ++i) {
    counter->ptrs[i] = NULL;
  }
  pthread_mutex_init(&counter->mtx, NULL);
}

uint64_t parallel_counter_read(parallel_counter_t *counter) {
  uint64_t sum = counter->final;

  pthread_mutex_lock(&counter->mtx);
  for (int i = 0; i < NR_THREADS; ++i) {
    if (counter->ptrs[i]) {
      sum += counter->ptrs[i]->counter;
    }
  }
  pthread_mutex_unlock(&counter->mtx);

  return sum;
}

void parallel_counter_register_thread(parallel_counter_t *counter, parallel_counter_tls_t *tls) {
  const int tid = parallel_thread_id() - 1;
  pthread_mutex_lock(&counter->mtx);
  counter->ptrs[tid] = tls;
  pthread_mutex_unlock(&counter->mtx);
}

void parallel_counter_unregister_thread(parallel_counter_t *counter, parallel_counter_tls_t *tls) {
  const int tid = parallel_thread_id() - 1;
  pthread_mutex_lock(&counter->mtx);
  counter->final += tls->counter;
  counter->ptrs[tid] = NULL;
  pthread_mutex_unlock(&counter->mtx);
}
