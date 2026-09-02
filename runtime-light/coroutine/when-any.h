// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <tuple>
#include <utility>

#include "common/type_traits/lazy-conditional.h"
#include "common/type_traits/lazy-identity.h"
#include "common/type_traits/lazy-invoke-result.h"
#include "runtime-light/coroutine/concepts.h"
#include "runtime-light/coroutine/detail/when-any.h"
#include "runtime-light/coroutine/type-traits.h"

namespace kphp::coro {

template<typename... Args>
requires(((kphp::coro::concepts::awaitable<Args> || kphp::coro::is_task_function_v<Args>) && ...))
[[nodiscard]] auto when_any(Args&&... args) noexcept {
  return detail::when_any::when_any_ready_awaitable<std::tuple<detail::when_any::when_any_task<typename kphp::coro::awaitable_traits<
      vk::lazy_conditional_t<kphp::coro::is_task_function_v<Args>, vk::lazy_invoke_result<Args>, vk::lazy_identity<Args>>>::awaiter_return_type>...>>{
      detail::when_any::make_when_any_task(std::forward<Args>(args))...};
}

} // namespace kphp::coro
