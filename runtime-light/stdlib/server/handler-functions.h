// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <functional>
#include <utility>

#include "runtime-light/coroutine/task.h"
#include "runtime-light/coroutine/type-traits.h"
#include "runtime-light/state/instance-state.h"

template<typename F, typename... Args>
requires(std::invocable<F, Args...>) void f$register_shutdown_function(F &&f, Args &&...args) noexcept {
  // it's a lambda coroutine, so:
  // 1. don't capture anything;
  // 2. parameters are passed by value.
  auto shutdown_function_task{std::invoke(
    [](F f, Args... args) noexcept -> task_t<void> {
      if constexpr (is_async_function_v<F, Args...>) {
        co_await std::invoke(std::move(f), std::move(args)...);
      } else {
        std::invoke(std::move(f), std::move(args)...);
      }
    },
    std::forward<F>(f), std::forward<Args>(args)...)};
  InstanceState::get().shutdown_functions.emplace_back(std::move(shutdown_function_task));
}

template<typename F>
void f$register_kphp_on_warning_callback(F && /*callback*/) {
  php_warning("called stub register_kphp_on_warning_callback");
}
