// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <optional>
#include <utility>

#include "runtime-light/coroutine/concepts.h"
#include "runtime-light/coroutine/detail/task-self-deleting.h"
#include "runtime-light/coroutine/event.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/coroutine/when-all.h"

namespace kphp::coro::detail::when_any {

template<typename result_type, kphp::coro::concepts::awaitable awaitable_type>
auto make_when_any_task(bool& first_completed, kphp::coro::event& notify, std::optional<result_type>& opt_result, awaitable_type awaitable) noexcept
    -> kphp::coro::task<> {
  auto result{co_await std::move(awaitable)};
  if (!std::exchange(first_completed, true)) {
    opt_result = std::move(result);
    notify.set();
  }
}

template<typename result_type, kphp::coro::concepts::awaitable... awaitable_types>
[[nodiscard]] auto make_when_any_controller_task(kphp::coro::event& notify, std::optional<result_type>& opt_result, awaitable_types... awaitables) noexcept
    -> kphp::coro::detail::task_self_deleting::task_self_deleting {
  bool first_completed{};
  co_await kphp::coro::when_all(make_when_any_task(first_completed, notify, opt_result, std::move(awaitables))...);
}

} // namespace kphp::coro::detail::when_any
