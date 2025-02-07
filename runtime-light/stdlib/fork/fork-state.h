// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <optional>
#include <utility>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/coroutine/shared-task.h"
#include "runtime-light/coroutine/task.h"

namespace kphp::forks {

inline constexpr int64_t INVALID_ID = -1;

} // namespace kphp::forks

class ForkInstanceState final : private vk::not_copyable {
  // This is a temporary solution to destroy the fork state after the first call to f$wait.
  // In the future, we plan to implement a reference-counted future<T> that will automatically
  // manage and destroy the fork state once all referencing futures have been destroyed.
  // NOTE: The implementation of f$wait_concurrently should be updated as well.
  enum class fork_state : uint8_t { running, awaited };
  static constexpr int64_t FORK_ID_INIT = 0;

  int64_t next_fork_id{FORK_ID_INIT + 1};
  // type erased tasks that represent forks
  kphp::stl::unordered_map<int64_t, std::pair<fork_state, shared_task_t<void>>, kphp::memory::script_allocator> forks;

  int64_t push_fork(shared_task_t<void> fork_task) noexcept {
    forks.emplace(next_fork_id, std::make_pair(fork_state::running, std::move(fork_task)));
    return next_fork_id++;
  }

  friend class start_fork_t;
  template<typename T>
  friend task_t<T> f$wait(int64_t, double) noexcept;
  friend task_t<bool> f$wait_concurrently(int64_t) noexcept;

public:
  int64_t current_id{FORK_ID_INIT};

  ForkInstanceState() noexcept = default;

  static ForkInstanceState &get() noexcept;

  std::optional<shared_task_t<void>> get_fork(int64_t fork_id) const noexcept {
    auto it{forks.find(fork_id)};
    if (it == forks.end() || it->second.first == fork_state::awaited) [[unlikely]] {
      return std::nullopt;
    }
    return it->second.second;
  }

  void erase_fork(int64_t fork_id) noexcept {
    auto it{forks.find(fork_id)};
    if (it == forks.end()) [[unlikely]] {
      return;
    }
    it->second = std::make_pair(fork_state::awaited, shared_task_t<void>{nullptr});
  }
};
