// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/threading/thread-id.h"

static thread_local int bicycle_thread_id;

int get_thread_id() {
  return bicycle_thread_id;
}

void set_thread_id(int new_thread_id) {
  bicycle_thread_id = new_thread_id;
}
