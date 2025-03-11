// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <utility>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
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
    Throwable thrown_exception;
    std::optional<shared_task_t<void>> opt_handle;
  };

private:
  static constexpr int64_t FORK_ID_INIT = 0;

  int64_t next_fork_id{FORK_ID_INIT};
  // type erased tasks that represent forks
  kphp::stl::unordered_map<int64_t, fork_info, kphp::memory::script_allocator> forks;

  int64_t push_fork(shared_task_t<void> fork_task) noexcept {
    forks.emplace(next_fork_id, fork_info{.thrown_exception = {}, .opt_handle = std::move(fork_task)});
    return next_fork_id++;
  }

  template<typename T>
  friend class start_fork_t;

public:
  int64_t current_id{FORK_ID_INIT};

  ForkInstanceState() noexcept = default;

  static ForkInstanceState &get() noexcept;

  std::optional<std::reference_wrapper<fork_info>> get_info(int64_t fork_id) noexcept {
    if (auto it{forks.find(fork_id)}; it != forks.end()) [[likely]] {
      return it->second;
    }
    return std::nullopt;
  }

  std::reference_wrapper<fork_info> current_info() noexcept {
    if (auto opt_fork_info{get_info(current_id)}; opt_fork_info.has_value()) [[likely]] {
      return *opt_fork_info;
    }
    php_critical_error("can't find info for current fork");
  }
};
