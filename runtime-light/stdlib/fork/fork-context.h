// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <optional>

#include "runtime-core/memory-resource/unsynchronized_pool_resource.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/fork/fork.h"
#include "runtime-light/utils/concepts.h"

class ForkComponentContext {
  template<hashable Key, typename Value>
  using unordered_map = memory_resource::stl::unordered_map<Key, Value, memory_resource::unsynchronized_pool_resource>;

  static constexpr auto FORK_ID_INIT = 1;

  unordered_map<int64_t, task_t<fork_result>> forks_;
  int64_t next_fork_id_{FORK_ID_INIT};

public:
  explicit ForkComponentContext(memory_resource::unsynchronized_pool_resource &) noexcept;

  static ForkComponentContext &get() noexcept;

  int64_t push_fork(task_t<fork_result> &&) noexcept;

  std::optional<task_t<fork_result>> pop_fork(int64_t) noexcept;
};
