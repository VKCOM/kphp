// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-light/coroutine/detail/when-all.h"
#include "runtime-light/coroutine/task.h"

namespace kphp::coro {

template<typename... return_types>
[[nodiscard]] auto when_all(kphp::coro::task<return_types>... tasks) noexcept {
  return detail::when_all::when_all_ready_awaitable<std::tuple<detail::when_all::when_all_task<return_types>...>>{
      detail::when_all::make_when_all_task(tasks)...};
}

} // namespace kphp::coro
