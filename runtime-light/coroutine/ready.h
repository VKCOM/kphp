// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <coroutine>
#include <utility>

namespace kphp::coro {

template<typename T = void>
class ready final {
private:
  T m_val;

public:
  constexpr explicit ready(T val) noexcept
      : m_val(std::move(val)) {}

  ready(const ready& other) = delete;
  constexpr ready(ready&& other) noexcept = default;

  ready& operator=(const ready& other) = delete;
  constexpr ready& operator=(ready&& other) noexcept = default;

  constexpr ~ready() = default;

  constexpr auto await_ready() const noexcept -> bool {
    return true;
  }

  constexpr auto await_suspend(std::coroutine_handle<> /*unused*/) const noexcept -> void {
    std::unreachable();
  }

  constexpr auto await_resume() noexcept -> T {
    return std::move(m_val);
  }
};

template<>
class ready<void> final {
public:
  constexpr auto await_ready() const noexcept -> bool {
    return true;
  }

  constexpr auto await_suspend(std::coroutine_handle<> /*unused*/) const noexcept -> void {
    std::unreachable();
  }

  constexpr auto await_resume() noexcept -> void {}
};

} // namespace kphp::coro
