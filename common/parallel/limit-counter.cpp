// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/parallel/limit-counter.h"

static inline void parallel_limit_counter_globalize_count(parallel_limit_counter_t* counter, parallel_limit_counter_tls_t* tls) {
  counter->global_count += tls->counter;
  tls->counter = 0;
  counter->global_reserve -= tls->counter_max;
  tls->counter_max = 0;
}

static inline void parallel_limit_counter_balance_count(parallel_limit_counter_t* counter, parallel_limit_counter_tls_t* tls) {
  tls->counter_max = counter->global_limit - counter->global_count - counter->global_reserve;
  tls->counter_max /= parallel_online_threads_count();

  if (tls->counter_max > counter->thread_max) {
    tls->counter_max = counter->thread_max;
  }

  counter->global_reserve += tls->counter_max;
  tls->counter = tls->counter_max / 2;
  if (tls->counter > counter->global_count) {
    tls->counter = counter->global_count;
  }

  counter->global_count -= tls->counter;
}

void parallel_limit_counter_register_thread(parallel_limit_counter_t* counter, parallel_limit_counter_tls_t* tls) {
  const int tid = parallel_thread_id() - 1;

  pthread_mutex_lock(&counter->mtx);

  counter->ptrs[tid] = tls;

  pthread_mutex_unlock(&counter->mtx);
}

void parallel_limit_counter_init(parallel_limit_counter_t* counter, uint64_t global_limit, uint64_t thread_max) {
  counter->global_limit = global_limit;
  counter->thread_max = thread_max;
}

void parallel_limit_counter_unregister_thread(parallel_limit_counter_t* counter, parallel_limit_counter_tls_t* tls) {
  const int tid = parallel_thread_id() - 1;

  pthread_mutex_lock(&counter->mtx);

  parallel_limit_counter_globalize_count(counter, tls);
  counter->ptrs[tid] = NULL;

  pthread_mutex_unlock(&counter->mtx);
}

bool parallel_limit_counter_add_slow_path(parallel_limit_counter_t* counter, parallel_limit_counter_tls_t* tls, uint64_t value) {
  pthread_mutex_lock(&counter->mtx);

  parallel_limit_counter_globalize_count(counter, tls);

  if ((counter->global_limit - counter->global_count - counter->global_reserve) < value) {
    pthread_mutex_unlock(&counter->mtx);
    return false;
  }

  counter->global_count += value;
  parallel_limit_counter_balance_count(counter, tls);

  pthread_mutex_unlock(&counter->mtx);

  return true;
}

bool parallel_limit_counter_sub_slow_path(parallel_limit_counter_t* counter, parallel_limit_counter_tls_t* tls, uint64_t value) {
  pthread_mutex_lock(&counter->mtx);

  parallel_limit_counter_globalize_count(counter, tls);

  if (counter->global_count < value) {
    pthread_mutex_unlock(&counter->mtx);
    return false;
  }

  counter->global_count -= value;
  parallel_limit_counter_balance_count(counter, tls);

  pthread_mutex_unlock(&counter->mtx);

  return true;
}

uint64_t parallel_limit_counter_read(parallel_limit_counter_t* counter) {
  pthread_mutex_lock(&counter->mtx);

  uint64_t sum = counter->global_count;
  for (int i = 0; i < NR_THREADS; ++i) {
    if (counter->ptrs[i]) {
      sum += counter->ptrs[i]->counter;
    }
  }

  pthread_mutex_unlock(&counter->mtx);

  return sum;
}
