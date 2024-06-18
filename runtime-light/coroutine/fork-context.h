// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-core/runtime-core.h"
#include "runtime-light/coroutine/fork-scheduler.h"
#include "runtime-light/coroutine/runtime-future.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/streams/streams.h"

struct KphpForkContext {

  static constexpr int64_t main_fork_id = 0;

  static KphpForkContext &get();

  KphpForkContext(memory_resource::unsynchronized_pool_resource &memory_pool)
    : scheduler(memory_pool) {}

  int64_t current_fork_id = main_fork_id;
  int64_t last_registered_fork_id = main_fork_id;

  fork_scheduler scheduler;
};
