// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <coroutine>
#include <tuple>
#include <type_traits>
#include <utility>

#include "runtime-light/coroutine/type-traits.h"

namespace kphp::coro {

namespace details {

template<typename F, typename... Args>
class lift_sync_awaiter {
  F m_func;
  std::tuple<Args...> m_args;

public:
  template<std::same_as<F> func_type, std::same_as<Args>... args_type>
  explicit lift_sync_awaiter(func_type&& f, args_type&&... args) noexcept
      : m_func(std::forward<func_type>(f)),
        m_args(std::forward<args_type>(args)...) {}

  lift_sync_awaiter(lift_sync_awaiter&& other) noexcept = default;
  lift_sync_awaiter& operator=(lift_sync_awaiter&& other) noexcept = default;
  ~lift_sync_awaiter() = default;

  lift_sync_awaiter(const lift_sync_awaiter&) = delete;
  lift_sync_awaiter& operator=(const lift_sync_awaiter&) = delete;

  constexpr auto await_ready() const noexcept -> bool {
    return true;
  }

  constexpr auto await_suspend(std::coroutine_handle<> /*unused*/) const noexcept -> void {
    std::unreachable();
  }

  auto await_resume() noexcept -> std::invoke_result_t<F, Args...> {
    return std::apply(std::move(m_func), std::move(m_args));
  }
};

} // namespace details

template<typename F, typename... Args>
requires std::invocable<F, Args...> && (!kphp::coro::is_async_function_v<F, Args...>)
auto lift_sync(F&& f, Args&&... args) noexcept {
  return details::lift_sync_awaiter<F, Args...>{std::forward<F>(f), std::forward<Args>(args)...};
}

} // namespace kphp::coro
