// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/coroutine/light-fork.h"
#include "runtime-light/coroutine/fork-context.h"

void light_fork::resume(int64_t id) {
  int64_t parent_fork_id = KphpForkContext::get().current_fork_id;
  KphpForkContext::get().current_fork_id = id;
  handle();
  KphpForkContext::get().current_fork_id = parent_fork_id;
}
