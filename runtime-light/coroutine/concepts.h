// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <coroutine>
#include <utility>

#include "runtime-light/coroutine/async-stack.h"

namespace kphp::coro::concepts {

namespace detail {

template<typename T, typename... Ts>
concept in_types = (std::same_as<T, Ts> || ...);

template<typename T>
concept valid_await_suspend_return_type = in_types<T, void, bool> || std::convertible_to<T, std::coroutine_handle<>>;

template<typename T>
concept awaiter = requires(T t, std::coroutine_handle<kphp::coro::async_stack_element> coro) {
  { t.await_ready() } noexcept -> std::same_as<bool>;
  { t.await_suspend(coro) } noexcept -> valid_await_suspend_return_type;
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

template<awaitable awaitable, typename = void>
struct awaitable_traits {};

namespace detail {

template<awaitable awaitable>
auto get_awaiter(awaitable&& value) noexcept {
  if constexpr (member_co_await_awaitable<awaitable>) {
    return std::forward<awaitable>(value).operator co_await();
  } else if constexpr (global_co_await_awaitable<awaitable>) {
    return operator co_await(std::forward<awaitable>(value));
  } else if constexpr (awaiter<awaitable>) {
    return std::forward<awaitable>(value);
  }
}

} // namespace detail

template<awaitable awaitable>
struct awaitable_traits<awaitable> {
  using awaiter_type = decltype(detail::get_awaiter(std::declval<awaitable>()));
  using awaiter_return_type = decltype(std::declval<awaiter_type>().await_resume());
};

} // namespace kphp::coro::concepts
