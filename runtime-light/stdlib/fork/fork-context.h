// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <optional>

#include "runtime-core/memory-resource/unsynchronized_pool_resource.h"
#include "runtime-core/utils/kphp-assert-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/fork/fork.h"
#include "runtime-light/utils/concepts.h"

class ForkComponentContext {
  template<hashable Key, typename Value>
  using unordered_map = memory_resource::stl::unordered_map<Key, Value, memory_resource::unsynchronized_pool_resource>;

  static constexpr auto FORK_ID_INIT = 0;

  unordered_map<int64_t, task_t<fork_result>> forks_;
  int64_t next_fork_id_{FORK_ID_INIT};
  ino64_t running_fork_id_{FORK_ID_INIT};

public:
  explicit ForkComponentContext(memory_resource::unsynchronized_pool_resource &memory_resource) noexcept
    : forks_(unordered_map<int64_t, task_t<fork_result>>::allocator_type{memory_resource}) {}

  static ForkComponentContext &get() noexcept;

  int64_t get_running_fork_id() const noexcept {
    return running_fork_id_;
  }

  void set_running_fork_id(int64_t running_fork_id) noexcept {
    running_fork_id_ = running_fork_id;
    php_debug("ForkComponentContext: execute fork %lu", running_fork_id_);
  }

  int64_t push_fork(task_t<fork_result> &&task) noexcept {
    const auto fork_id{++next_fork_id_};
    forks_.emplace(fork_id, std::move(task));
    php_debug("ForkComponentContext: push fork %" PRId64, fork_id);
    return fork_id;
  }

  std::optional<task_t<fork_result>> pop_fork(int64_t fork_id) noexcept {
    if (const auto it_fork{forks_.find(fork_id)}; it_fork != forks_.cend()) {
      php_debug("ForkComponentContext: pop fork %" PRId64, fork_id);
      auto fork{std::move(it_fork->second)};
      forks_.erase(it_fork);
      return {std::move(fork)};
    }
    return std::nullopt;
  }
};
