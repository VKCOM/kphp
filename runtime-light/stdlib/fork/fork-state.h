// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <utility>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/coroutine/concepts.h"
#include "runtime-light/coroutine/shared-task.h"
#include "runtime-light/coroutine/type-traits.h"
#include "runtime-light/stdlib/diagnostics/exception-types.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/fork/fork-storage.h"

namespace kphp::forks {

inline constexpr int64_t INVALID_ID = -1;

} // namespace kphp::forks

struct ForkInstanceState final : private vk::not_copyable {
  // This is a temporary solution to destroy the fork state after the first call to f$wait.
  // In the future, we plan to implement a reference-counted future<T> that will automatically
  // manage and destroy the fork state once all referencing futures have been destroyed.
  struct fork_info final {
    bool awaited{};
    Throwable thrown_exception;
    std::optional<kphp::coro::shared_task<kphp::forks::details::storage>> opt_handle;
  };

private:
  static constexpr auto FORK_ID_INIT{std::numeric_limits<int64_t>::max()};

  int64_t next_fork_id{FORK_ID_INIT};
  // type erased tasks that represent forks
  kphp::stl::unordered_map<int64_t, fork_info, kphp::memory::script_allocator> forks;

public:
  int64_t current_id{FORK_ID_INIT};

  ForkInstanceState() noexcept = default;

  static ForkInstanceState& get() noexcept;

  template<kphp::coro::concepts::awaitable awaitable_type>
  auto create_fork(awaitable_type awaitable) noexcept -> std::pair<int64_t, kphp::coro::shared_task<kphp::forks::details::storage>>;

  std::optional<std::reference_wrapper<fork_info>> get_info(int64_t fork_id) noexcept {
    if (auto it{forks.find(fork_id)}; it != forks.end()) [[likely]] {
      return it->second;
    }
    return std::nullopt;
  }

  std::reference_wrapper<fork_info> current_info() noexcept {
    auto opt_fork_info{get_info(current_id)};
    kphp::log::assertion(opt_fork_info.has_value());
    return *opt_fork_info;
  }
};

template<kphp::coro::concepts::awaitable awaitable_type>
auto ForkInstanceState::create_fork(awaitable_type awaitable) noexcept -> std::pair<int64_t, kphp::coro::shared_task<kphp::forks::details::storage>> {
  static constexpr auto fork_coroutine{
      [](awaitable_type awaitable, int64_t fork_id, ForkInstanceState& fork_instance_state) noexcept -> kphp::coro::shared_task<kphp::forks::details::storage> {
        fork_instance_state.current_id = fork_id;

        kphp::forks::details::storage s{};
        if constexpr (std::same_as<typename kphp::coro::awaitable_traits<awaitable_type>::awaiter_return_type, void>) {
          co_await std::move(awaitable);
          s.store();
        } else {
          s.store<typename kphp::coro::awaitable_traits<awaitable_type>::awaiter_return_type>(co_await std::move(awaitable));
        }
        co_return s;
      }};

  const int64_t fork_id{next_fork_id--};
  auto fork_task{std::invoke(fork_coroutine, std::move(awaitable), fork_id, *this)};
  forks.emplace(fork_id, fork_info{.awaited = {}, .thrown_exception = {}, .opt_handle = fork_task});
  return std::make_pair(fork_id, std::move(fork_task));
}
