// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

#include "runtime-light/coroutine/concepts.h"
#include "runtime-light/coroutine/detail/when-any.h"
#include "runtime-light/coroutine/event.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/coroutine/type-traits.h"
#include "runtime-light/utils/logs.h"

namespace kphp::coro {

template<kphp::coro::concepts::awaitable... awaitable_types>
[[nodiscard]] auto when_any(awaitable_types... awaitables) noexcept
    -> kphp::coro::task<std::variant<std::remove_cvref_t<typename kphp::coro::awaitable_traits<awaitable_types>::awaiter_return_type>...>> {
  kphp::coro::event notify{};
  std::optional<std::variant<std::remove_cvref_t<typename kphp::coro::awaitable_traits<awaitable_types>::awaiter_return_type>...>> opt_result{};
  auto controller_task{detail::when_any::make_when_any_controller_task(notify, opt_result, std::move(awaitables)...)};

  controller_task.get_handle().resume();
  co_await notify;
  // after successfully awaiting any of the provided awaitables, we need to destroy the controller task since
  // we don't want it to wait for remaining awaitable
  controller_task.get_handle().destroy();
  kphp::log::assertion(opt_result.has_value());
  co_return std::move(*opt_result);
}

} // namespace kphp::coro
