// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <utility>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/allocator/allocator.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/utils/concepts.h"

inline constexpr int64_t INVALID_FORK_ID = -1;

class ForkInstanceState final : private vk::not_copyable {
  template<hashable Key, typename Value>
  using unordered_map = kphp::stl::unordered_map<Key, Value, kphp::allocator::script_allocator>;

  static constexpr auto FORK_ID_INIT = 0;
  // type erased tasks that represent forks
  unordered_map<int64_t, task_t<void>> forks;
  int64_t next_fork_id{FORK_ID_INIT + 1};

  int64_t push_fork(task_t<void> task) noexcept {
    return forks.emplace(next_fork_id, std::move(task)), next_fork_id++;
  }

  task_t<void> pop_fork(int64_t fork_id) noexcept {
    const auto it_fork{forks.find(fork_id)};
    if (it_fork == forks.end()) {
      php_critical_error("can't find fork %" PRId64, fork_id);
    }
    auto fork{std::move(it_fork->second)};
    forks.erase(it_fork);
    return fork;
  }

  friend class start_fork_t;
  template<typename>
  friend class wait_fork_t;

public:
  int64_t running_fork_id{FORK_ID_INIT};

  ForkInstanceState() noexcept = default;

  static ForkInstanceState &get() noexcept;

  bool contains(int64_t fork_id) const noexcept {
    return forks.contains(fork_id);
  }
};
