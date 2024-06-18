// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-core/runtime-core.h"
#include "runtime-light/coroutine/runtime-future.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/streams/streams.h"

// Type erasure of stored object
struct fork_result {
  small_object_storage<sizeof(mixed)> storage;

  template<typename T>
  fork_result(T &&t) {
    storage.emplace<std::remove_cvref_t<T>>(std::forward<T>(t));
  }

  template<typename T>
  T get() {
    return *storage.get<T>();
  }
};

// Universal task type for any type of fork
struct light_fork {
  // storage for result and coro-frame pointer
  task_t<fork_result> task;
  // last nested suspend point to resume
  std::coroutine_handle<> handle;

  light_fork() = default;

  light_fork(task_t<fork_result> &&other)
    : task(std::move(other)) {
    handle = task.get_handle();
  }

  template<typename T>
  T get_fork_result() {
    return task.get_result().get<T>();
  }

  void resume(int64_t id);
};
