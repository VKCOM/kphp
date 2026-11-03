// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

#include "runtime-light/coroutine/concepts.h"
#include "runtime-light/coroutine/detached-task.h"
#include "runtime-light/coroutine/detail/type-traits-utils.h"
#include "runtime-light/coroutine/shared-task.h"
#include "runtime-light/coroutine/task.h"

namespace kphp::coro {

template<typename F, typename... Args>
inline constexpr bool is_task_function_v = requires { requires std::invocable<F, Args...> && is_task_v<std::invoke_result_t<F, Args...>>; };

template<typename F, typename... Args>
inline constexpr bool is_async_function_v = is_task_function_v<F, Args...> || requires {
  requires std::invocable<F, Args...>;
  { static_cast<kphp::coro::shared_task<>>(std::declval<std::invoke_result_t<F, Args...>>()) };
} || requires {
  { static_cast<kphp::coro::detached_task<>>(std::declval<std::invoke_result_t<F, Args...>>()) };
};

// ================================================================================================

template<typename F, typename... Args>
requires std::invocable<F, Args...>
using async_function_return_type_t = kphp::coro::detail::async_function_return_type<F, Args...>::type;

// ================================================================================================

template<kphp::coro::concepts::awaitable awaitable_type, typename = void>
struct awaitable_traits {};

template<kphp::coro::concepts::awaitable awaitable_type>
struct awaitable_traits<awaitable_type> {
  using awaiter_type = decltype(kphp::coro::concepts::detail::get_awaiter(std::declval<awaitable_type>()));
  using awaiter_return_type = decltype(std::declval<awaiter_type>().await_resume());
};

// ================================================================================================

template<kphp::coro::concepts::coroutine coroutine_type, typename = void>
struct coroutine_traits {};

template<kphp::coro::concepts::coroutine coroutine_type>
struct coroutine_traits<coroutine_type> {
  using return_type = awaitable_traits<coroutine_type>::awaiter_return_type;
};

} // namespace kphp::coro
