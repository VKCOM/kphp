#include "compiler/threading/thread-id.h"

static __thread int bicycle_thread_id;

int get_thread_id() {
  return bicycle_thread_id;
}

void set_thread_id(int new_thread_id) {
  bicycle_thread_id = new_thread_id;
}
