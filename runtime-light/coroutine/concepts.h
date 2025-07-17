// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <coroutine>
#include <utility>

#include "runtime-light/coroutine/shared-task.h"
#include "runtime-light/coroutine/task.h"

namespace kphp::coro::concepts {

namespace detail {

template<typename T, typename... Ts>
concept in_types = (std::same_as<T, Ts> || ...);

template<typename T>
concept valid_await_suspend_return_type = in_types<void, bool> || std::convertible_to<T, std::coroutine_handle<>>;

template<typename T>
concept awaiter = requires(T t) {
  { t.await_ready() } noexcept -> std::same_as<bool>;
  // TODO find a way to add 'await_suspend' here. for now there is a problem that we have two kinds of await_suspend:
  // 1. one that accepts type erase coroutine handle: std::coroutine_handle<>
  // 2. one that accepts generic coroutine handle: std::coroutine_handle<promise_type> where promise_type
  //    is inherited from kphp::coro::async_stack_element
  { t.await_resume() } noexcept;
};

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
