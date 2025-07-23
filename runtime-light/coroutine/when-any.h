// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <tuple>
#include <utility>

#include "runtime-light/coroutine/concepts.h"
#include "runtime-light/coroutine/detail/when-any.h"
#include "runtime-light/coroutine/type-traits.h"

namespace kphp::coro {

template<kphp::coro::concepts::awaitable... awaitable_types>
[[nodiscard]] auto when_any(awaitable_types... awaitables) noexcept {
  return detail::when_any::when_any_ready_awaitable<
      std::tuple<detail::when_any::when_any_task<typename kphp::coro::awaitable_traits<awaitable_types>::awaiter_return_type>...>>{
      detail::when_any::make_when_any_task(std::move(awaitables))...};
}

} // namespace kphp::coro
