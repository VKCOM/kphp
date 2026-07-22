// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <coroutine>
#include <utility>

namespace kphp::coro {

template<typename T = void>
class ready final {
  T m_val;

public:
  template<typename U>
  requires(std::same_as<T, std::remove_cvref_t<U>>)
  constexpr explicit ready(U&& val) noexcept
      : m_val{std::forward<U>(val)} {}

  template<typename U>
  requires(!std::same_as<T, U> && std::constructible_from<T, U>)
  constexpr explicit ready(ready<U>&& other) noexcept
      : m_val{std::move(other).result()} {}

  ready(const ready& other) = delete;
  constexpr ready(ready&& other) noexcept = default;

  ready& operator=(const ready& other) = delete;
  constexpr ready& operator=(ready&& other) noexcept = default;

  constexpr ~ready() = default;

  template<typename Self>
  constexpr decltype(auto) result(this Self&& self) noexcept {
    return std::forward<Self>(self).m_val;
  }

  constexpr auto operator co_await() && noexcept {
    struct awaiter {
      T m_val;

      constexpr auto await_ready() const noexcept -> bool {
        return true;
      }

      auto await_suspend(std::coroutine_handle<> /*unused*/) const noexcept -> void {
        std::unreachable();
      }

      constexpr auto await_resume() noexcept -> T {
        return std::move(m_val);
      }
    };

    return awaiter{std::move(m_val)};
  }
};

template<>
class ready<void> final {
public:
  constexpr auto await_ready() const noexcept -> bool {
    return true;
  }

  auto await_suspend(std::coroutine_handle<> /*unused*/) const noexcept -> void {
    std::unreachable();
  }

  constexpr auto await_resume() noexcept -> void {}
};

} // namespace kphp::coro
