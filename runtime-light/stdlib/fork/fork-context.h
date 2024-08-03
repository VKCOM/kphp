// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-core/memory-resource/unsynchronized_pool_resource.h"
#include "runtime-core/utils/kphp-assert-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/fork/fork.h"
#include "runtime-light/utils/concepts.h"

constexpr int64_t INVALID_FORK_ID = -1;

class ForkComponentContext {
  template<hashable Key, typename Value>
  using unordered_map = memory_resource::stl::unordered_map<Key, Value, memory_resource::unsynchronized_pool_resource>;

  static constexpr auto FORK_ID_INIT = 1;

  unordered_map<int64_t, task_t<fork_result>> forks;
  int64_t next_fork_id{FORK_ID_INIT};

public:
  explicit ForkComponentContext(memory_resource::unsynchronized_pool_resource &memory_resource) noexcept
    : forks(unordered_map<int64_t, task_t<fork_result>>::allocator_type{memory_resource}) {}

  static ForkComponentContext &get() noexcept;

  bool contains(int64_t fork_id) const noexcept {
    return forks.contains(fork_id);
  }

  int64_t push_fork(task_t<fork_result> &&task) noexcept {
    const auto fork_id{next_fork_id++};
    forks.emplace(fork_id, std::move(task));
    return fork_id;
  }

  task_t<fork_result> &get_fork(int64_t fork_id) noexcept {
    const auto it_fork{forks.find(fork_id)};
    if (it_fork == forks.end()) {
      php_critical_error("can't find fork %" PRId64, fork_id);
    }
    return it_fork->second;
  }

  void pop_fork(int64_t fork_id) noexcept {
    forks.erase(fork_id);
  }
};
