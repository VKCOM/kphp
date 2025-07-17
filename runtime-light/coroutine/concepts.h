// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <coroutine>
#include <utility>

#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/coroutine/shared-task.h"
#include "runtime-light/coroutine/task.h"

namespace kphp::coro::concepts {

namespace detail {

template<typename type, typename... types>
concept in_types = (std::same_as<type, types> || ...);

template<typename type>
concept awaiter_base = requires(type t) {
  { t.await_ready() } noexcept -> std::same_as<bool>;
  { t.await_resume() } noexcept;
};

template<typename type>
concept awaiter = awaiter_base<type> && (requires(type t, std::coroutine_handle<> coroutine) {
  { t.await_suspend(coroutine) } noexcept;
} || requires(type t, std::coroutine_handle<kphp::coro::async_stack_element> coroutine) {
  { t.await_suspend(coroutine) } noexcept;
});

template<typename T>
concept member_co_await_awaitable = requires(T t) {
  { t.operator co_await() } -> awaiter;
};

template<typename T>
concept global_co_await_awaitable = requires(T t) {
  { operator co_await(t) } -> awaiter;
};

} // namespace detail

template<typename T>
concept awaitable = detail::member_co_await_awaitable<T> || detail::global_co_await_awaitable<T> || detail::awaiter<T>;

namespace detail {

template<awaitable awaitable_type>
auto get_awaiter(awaitable_type&& value) noexcept {
  if constexpr (member_co_await_awaitable<awaitable_type>) {
    return std::forward<awaitable_type>(value).operator co_await();
  } else if constexpr (global_co_await_awaitable<awaitable_type>) {
    return operator co_await(std::forward<awaitable_type>(value));
  } else if constexpr (awaiter<awaitable_type>) {
    return std::forward<awaitable_type>(value);
  }
}

} // namespace detail

template<typename T>
concept coroutine = awaitable<T> && (requires {
  { static_cast<kphp::coro::task<>>(std::declval<T>()) };
} || requires {
  { static_cast<kphp::coro::shared_task<>>(std::declval<T>()) };
});

} // namespace kphp::coro::concepts
