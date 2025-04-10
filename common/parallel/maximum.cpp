// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/parallel/maximum.h"

static inline void parallel_maximum_globalize_maximum(parallel_maximum_t* maximum, parallel_maximum_tls_t* tls) {
  maximum->global_count += tls->counter;
  tls->counter = 0;
  maximum->global_reserve -= tls->counter_max;
  tls->counter_max = 0;
}

static inline void parallel_maximum_balance_maximum(parallel_maximum_t* maximum, parallel_maximum_tls_t* tls) {
  tls->counter_max = maximum->thread_max;
  tls->counter = maximum->thread_max / 2;
  maximum->global_reserve += maximum->thread_max;

  if (tls->counter > maximum->global_count) {
    tls->counter = maximum->global_count;
  }

  maximum->global_count -= tls->counter;
}

void parallel_maximum_register_thread(parallel_maximum_t* maximum, parallel_maximum_tls_t* tls) {
  (void)maximum;
  (void)tls;
}

void parallel_maximum_unregister_thread(parallel_maximum_t* maximum, parallel_maximum_tls_t* tls) {
  pthread_mutex_lock(&maximum->mtx);

  parallel_maximum_globalize_maximum(maximum, tls);

  pthread_mutex_unlock(&maximum->mtx);
}

void parallel_maximum_init(parallel_maximum_t* maximum, uint64_t thread_max) {
  maximum->global_count = 0;
  maximum->global_reserve = 0;
  maximum->global_maximum = 0;
  maximum->thread_max = thread_max;
}

void parallel_maximum_add_slow_path(parallel_maximum_t* maximum, parallel_maximum_tls_t* tls, uint64_t value) {
  pthread_mutex_lock(&maximum->mtx);

  parallel_maximum_globalize_maximum(maximum, tls);

  if (maximum->thread_max < value) {
    pthread_mutex_unlock(&maximum->mtx);
    return;
  }

  maximum->global_count += value;
  parallel_maximum_balance_maximum(maximum, tls);

  if (maximum->global_maximum < maximum->global_count + maximum->global_reserve) {
    maximum->global_maximum = maximum->global_count + maximum->global_reserve;
  }

  pthread_mutex_unlock(&maximum->mtx);
}

void parallel_maximum_sub_slow_path(parallel_maximum_t* maximum, parallel_maximum_tls_t* tls, uint64_t value) {
  pthread_mutex_lock(&maximum->mtx);

  parallel_maximum_globalize_maximum(maximum, tls);

  if (maximum->global_count < value) {
    pthread_mutex_unlock(&maximum->mtx);
    return;
  }

  maximum->global_count -= value;
  parallel_maximum_balance_maximum(maximum, tls);

  pthread_mutex_unlock(&maximum->mtx);
}
