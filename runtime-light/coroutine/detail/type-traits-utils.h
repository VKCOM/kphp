// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <type_traits>

#include "runtime-light/coroutine/detached-task.h"
#include "runtime-light/coroutine/shared-task.h"
#include "runtime-light/coroutine/task.h"

namespace kphp::coro {

namespace detail {

template<typename F, typename... Args>
requires std::invocable<F, Args...>
class async_function_return_type {
  using return_type = std::invoke_result_t<F, Args...>;

  template<typename U>
  struct task_inner {
    using type = U;
  };

  template<typename U>
  struct task_inner<kphp::coro::task<U>> {
    using type = U;
  };

  template<typename U>
  struct task_inner<kphp::coro::detached_task<U>> {
    using type = U;
  };

  template<typename U>
  struct task_inner<kphp::coro::shared_task<U>> {
    using type = U;
  };

public:
  using type = task_inner<return_type>::type;
};

template<typename T>
struct is_task : std::false_type {};

template<typename T>
struct is_task<kphp::coro::task<T>> : std::true_type {};

} // namespace detail

template<typename T>
inline constexpr bool is_task_v = kphp::coro::detail::is_task<T>::value;

} // namespace kphp::coro
