// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

#include "runtime-light/coroutine/shared_task.h"
#include "runtime-light/coroutine/task.h"

template<typename F, typename... Args>
requires std::invocable<F, Args...> inline constexpr bool is_async_function_v = requires {
  {static_cast<task_t<void>>(std::declval<std::invoke_result_t<F, Args...>>())};
}
|| requires {
  {static_cast<shared_task_t<void>>(std::declval<std::invoke_result_t<F, Args...>>())};
};

// ================================================================================================

template<typename F, typename... Args>
requires std::invocable<F, Args...> class async_function_unwrapped_return_type {
  using return_type = std::invoke_result_t<F, Args...>;

  template<typename U>
  struct task_inner {
    using type = U;
  };

  template<typename U>
  struct task_inner<task_t<U>> {
    using type = U;
  };

  template<typename U>
  struct task_inner<shared_task_t<U>> {
    using type = U;
  };

public:
  using type = task_inner<return_type>::type;
};

template<typename F, typename... Args>
requires std::invocable<F, Args...> using async_function_unwrapped_return_type_t = async_function_unwrapped_return_type<F, Args...>::type;
