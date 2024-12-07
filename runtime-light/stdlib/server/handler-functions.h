// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <functional>
#include <utility>

#include "runtime-light/coroutine/task.h"
#include "runtime-light/state/instance-state.h"

template<typename F, typename... Args>
requires(std::invocable<F, Args...>) void f$register_shutdown_function(F &&f, Args &&...args) noexcept {
  // it's crucial that we don't use lambda capturing and accept parameters by value instead.
  // the reason for that is coroutine's lifetime
  auto shutdown_function_task{[](F f, Args... args) noexcept -> task_t<void> {
    if constexpr (is_async_function_v<F, Args...>) {
      co_await std::invoke(f, args...);
    } else {
      std::invoke(f, args...);
    }
  }(std::forward<F>(f), std::forward<Args>(args)...)};
  InstanceState::get().shutdown_functions.emplace_back(std::move(shutdown_function_task));
}
