// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

#include "runtime-light/coroutine/detail/when-any.h"
#include "runtime-light/coroutine/event.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/utils/logs.h"

namespace kphp::coro {

template<typename... return_types>
[[nodiscard]] auto when_any(kphp::coro::task<return_types>... tasks) noexcept -> kphp::coro::task<std::variant<std::remove_cvref_t<return_types>...>> {
  kphp::coro::event notify{};
  std::optional<std::variant<std::remove_cvref_t<return_types>...>> opt_result{};
  auto controller_task{detail::when_any::make_when_any_tuple_controller_task(notify, opt_result, std::move(tasks)...)};
  controller_task.get_handle().resume(); // TODO async stack

  co_await notify;
  kphp::log::assertion(opt_result.has_value());
  co_return std::move(*opt_result);
}

} // namespace kphp::coro
