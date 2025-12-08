// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <coroutine>
#include <utility>

#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/coroutine/coroutine-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::coro {

class event {
  // 1) nullptr => not set
  // 2) awaiter* => linked list of awaiters waiting for the event to trigger
  // 3) this => The event is triggered and all awaiters are resumed
  void* m_state{};
  kphp::coro::async_stack_root* m_async_stack_root{};

  struct awaiter {
    event& m_event;
    bool m_suspended{};
    std::coroutine_handle<> m_awaiting_coroutine;
    CoroutineInstanceState& m_coroutine_state;

    awaiter* m_next{};
    awaiter* m_prev{};

    explicit awaiter(event& event) noexcept
        : m_event(event),
          m_coroutine_state(CoroutineInstanceState::get()) {}

    awaiter(const awaiter&) = delete;
    awaiter(awaiter&&) = delete;
    auto operator=(const awaiter&) -> awaiter& = delete;
    auto operator=(awaiter&&) -> awaiter& = delete;

    ~awaiter() {
      if (m_suspended) {
        cancel_awaiter();
      }
    }

    auto await_ready() const noexcept -> bool;
    template<std::derived_from<kphp::coro::async_stack_element> caller_promise_type>
    auto await_suspend(std::coroutine_handle<caller_promise_type> awaiting_coroutine) noexcept -> void;
    auto await_resume() noexcept -> void;

  private:
    auto cancel_awaiter() noexcept -> void;
  };

public:
  auto set() noexcept -> void;

  auto unset() noexcept -> void;

  auto is_set() const noexcept -> bool;

  auto operator co_await() noexcept;
};

inline auto event::awaiter::cancel_awaiter() noexcept -> void {
  if (m_next != nullptr) {
    m_next->m_prev = m_prev;
  }
  if (m_prev != nullptr) {
    m_prev->m_next = m_next;
  } else {
    // we are the head of the awaiters list, so we need to update the head
    m_event.m_state = m_next;
  }
  m_next = nullptr;
  m_prev = nullptr;
}

inline auto event::awaiter::await_ready() const noexcept -> bool {
  return m_event.is_set();
}

template<std::derived_from<kphp::coro::async_stack_element> caller_promise_type>
auto event::awaiter::await_suspend(std::coroutine_handle<caller_promise_type> awaiting_coroutine) noexcept -> void {
  m_event.m_async_stack_root = m_coroutine_state.current_async_stack_root;

  m_suspended = true;
  m_awaiting_coroutine = awaiting_coroutine;

  m_next = static_cast<event::awaiter*>(m_event.m_state);

  // ensure that the event isn't triggered
  kphp::log::assertion(reinterpret_cast<event*>(m_next) != std::addressof(m_event));

  if (m_next != nullptr) {
    m_next->m_prev = this;
  }
  m_event.m_state = this;
}

inline auto event::awaiter::await_resume() noexcept -> void {
  if (std::exchange(m_suspended, false)) {
    kphp::coro::preparation_for_resume(m_event.m_async_stack_root, STACK_FRAME_ADDRESS, m_coroutine_state);
  }
}

inline auto event::set() noexcept -> void {
  void* prev_value{std::exchange(m_state, this)};
  if (prev_value == this || prev_value == nullptr) [[unlikely]] {
    return;
  }
  auto* awaiter{static_cast<event::awaiter*>(prev_value)};
  while (awaiter != nullptr) {
    auto* next{awaiter->m_next};
    awaiter->m_awaiting_coroutine.resume();
    m_async_stack_root->stop_sync_stack_frame = nullptr;
    awaiter = next;
  }
}

inline auto event::unset() noexcept -> void {
  if (m_state == this) {
    m_state = nullptr;
  }
}

inline auto event::is_set() const noexcept -> bool {
  return m_state == this;
}

inline auto event::operator co_await() noexcept {
  return event::awaiter{*this};
}

} // namespace kphp::coro
