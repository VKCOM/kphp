// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/parallel/thread-id.h"

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>

struct thread_state {
  bool running;
  pthread_t id;
};
typedef struct thread_state thread_state_t;

static __thread int thread_id_;
static thread_state_t thread_id_map_[NR_THREADS];
static pthread_mutex_t thread_id_map_lock = PTHREAD_MUTEX_INITIALIZER;
static int parallel_online_threads_count_;

void parallel_register_thread() {
  assert(!thread_id_);

  const pthread_t tid = pthread_self();
  pthread_mutex_lock(&thread_id_map_lock);
  for (int i = 0; i < NR_THREADS; ++i) {
    if (!thread_id_map_[i].running) {
      thread_id_map_[i].running = true;
      thread_id_map_[i].id = tid;
      thread_id_ = i + 1;
      ++parallel_online_threads_count_;
      break;
    }
  }
  pthread_mutex_unlock(&thread_id_map_lock);

  assert(thread_id_ && "Parallel thread limit exceeded");
}

void parallel_unregister_thread() {
  assert(thread_id_);

  pthread_mutex_lock(&thread_id_map_lock);
  thread_id_map_[thread_id_ - 1].running = false;
  --parallel_online_threads_count_;
  pthread_mutex_unlock(&thread_id_map_lock);
}

int parallel_online_threads_count() {
  return parallel_online_threads_count_;
}

int parallel_thread_id() {
  assert(thread_id_);

  return thread_id_;
}
