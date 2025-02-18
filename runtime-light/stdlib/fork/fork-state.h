// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <optional>
#include <utility>
#include <variant>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/coroutine/shared-task.h"
#include "runtime-light/stdlib/diagnostics/exception-types.h"

namespace kphp::forks {

inline constexpr int64_t INVALID_ID = -1;

} // namespace kphp::forks

struct ForkInstanceState final : private vk::not_copyable {
  // This is a temporary solution to destroy the fork state after the first call to f$wait.
  // In the future, we plan to implement a reference-counted future<T> that will automatically
  // manage and destroy the fork state once all referencing futures have been destroyed.
  struct fork_info final {
    std::optional<Throwable> thrown_exception;
    std::variant<std::monostate, shared_task_t<void>> handle;
  };

private:
  static constexpr int64_t FORK_ID_INIT = 0;

  int64_t next_fork_id{FORK_ID_INIT + 1};
  // type erased tasks that represent forks
  kphp::stl::unordered_map<int64_t, fork_info, kphp::memory::script_allocator> forks;

  int64_t push_fork(shared_task_t<void> fork_task) noexcept {
    forks.emplace(next_fork_id, fork_info{.thrown_exception = std::nullopt, .handle = std::move(fork_task)});
    return next_fork_id++;
  }

  template<typename T>
  friend class start_fork_t;

public:
  int64_t current_id{FORK_ID_INIT};

  ForkInstanceState() noexcept = default;

  static ForkInstanceState &get() noexcept;

  std::optional<fork_info> get_info(int64_t fork_id) const noexcept {
    if (auto it{forks.find(fork_id)}; it != forks.end()) [[likely]] {
      return it->second;
    }
    return std::nullopt;
  }

  void clear_fork(int64_t fork_id) noexcept {
    auto it{forks.find(fork_id)};
    if (it == forks.end()) [[unlikely]] {
      return;
    }
    it->second.handle.emplace<std::monostate>();
  }
};
