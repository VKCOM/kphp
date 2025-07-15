// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <coroutine>
#include <utility>

namespace kphp::coro {

class event {
  // 1) nullptr == not set
  // 2) awaiter* == linked list of awaiters waiting for the event to trigger
  // 3) this == The event is triggered and all awaiters are resumed
  void* m_state{};

  struct awaiter {
    event& m_event;
    awaiter* m_next{};
    std::coroutine_handle<> m_awaiting_coroutine;

    explicit awaiter(event& event) noexcept
        : m_event(event) {}

    auto await_ready() const noexcept -> bool;
    auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> bool;
    constexpr auto await_resume() const noexcept;
  };

public:
  auto set() noexcept -> void;

  auto is_set() const noexcept -> bool;

  auto operator co_await() noexcept;
};

inline auto event::awaiter::await_ready() const noexcept -> bool {
  return m_event.is_set();
}

inline auto event::awaiter::await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> bool {
  if (m_event.is_set()) {
    return false;
  }
  m_awaiting_coroutine = awaiting_coroutine;
  m_next = static_cast<event::awaiter*>(m_event.m_state);
  m_event.m_state = this;
  return true;
}

inline constexpr auto event::awaiter::await_resume() const noexcept {}

inline auto event::set() noexcept -> void {
  void* prev_value{std::exchange(m_state, this)};
  if (prev_value == this) [[unlikely]] {
    return;
  }

  for (auto* awaiter{static_cast<event::awaiter*>(prev_value)}; awaiter != nullptr; awaiter = awaiter->m_next) {
    awaiter->m_awaiting_coroutine.resume(); // TODO async stack root
  }
}

inline auto event::is_set() const noexcept -> bool {
  return m_state == this;
}

inline auto event::operator co_await() noexcept {
  return event::awaiter{*this};
}

} // namespace kphp::coro
