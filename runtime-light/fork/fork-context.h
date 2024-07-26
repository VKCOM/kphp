// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-core/memory-resource/unsynchronized_pool_resource.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/fork/fork.h"

class ForkComponentContext {
  template<typename Key, typename Value>
  using unordered_map = memory_resource::stl::unordered_map<Key, Value, memory_resource::unsynchronized_pool_resource>;

  unordered_map<int64_t, task_t<fork_result>> forks_;
  int64_t next_fork_id_{};

public:
  explicit ForkComponentContext(memory_resource::unsynchronized_pool_resource &) noexcept;

  static ForkComponentContext &get() noexcept;

  int64_t register_fork(task_t<fork_result> &&) noexcept;

  void unregister_fork(int64_t) noexcept;
};
