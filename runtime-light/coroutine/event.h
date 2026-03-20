// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <coroutine>
#include <memory>
#include <utility>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator-managed.h"
#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/coroutine/coroutine-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::coro {

class event {
  struct state_handle : kphp::memory::script_allocator_managed, vk::not_copyable {
    // 1) nullptr => not set
    // 2) awaiter* => linked list of awaiters waiting for the event to trigger
    // 3) this => The event is triggered and all awaiters are resumed
    void* m_state{};
  };

  std::unique_ptr<state_handle> m_handle;

  struct awaiter {
    event& m_event;
    bool m_suspended{};
    std::coroutine_handle<> m_awaiting_coroutine;
    kphp::coro::async_stack_root& m_async_stack_root;
    kphp::coro::async_stack_frame* m_caller_async_stack_frame{};

    awaiter* m_next{};
    awaiter* m_prev{};

    explicit awaiter(event& event) noexcept
        : m_event(event),
          m_async_stack_root(CoroutineInstanceState::get().coroutine_stack_root) {}

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
  event() noexcept {
    m_handle = std::make_unique<state_handle>();
    kphp::log::assertion(m_handle != nullptr);
  }

  event(event&& other) noexcept
      : m_handle(std::move(other.m_handle)) {}

  event& operator=(event&& other) noexcept {
    if (this != std::addressof(other)) {
      m_handle = std::move(other.m_handle);
    }
    return *this;
  }

  ~event() = default;

  event(const event&) = delete;
  event& operator=(const event&) = delete;

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
    m_event.m_handle->m_state = m_next;
  }
  m_next = nullptr;
  m_prev = nullptr;
}

inline auto event::awaiter::await_ready() const noexcept -> bool {
  return m_event.is_set();
}

template<std::derived_from<kphp::coro::async_stack_element> caller_promise_type>
auto event::awaiter::await_suspend(std::coroutine_handle<caller_promise_type> awaiting_coroutine) noexcept -> void {
  // save caller's async stack frame
  m_caller_async_stack_frame = m_async_stack_root.top_async_stack_frame;

  m_suspended = true;
  m_awaiting_coroutine = awaiting_coroutine;

  m_next = static_cast<event::awaiter*>(m_event.m_handle->m_state);

  // ensure that the event isn't triggered
  kphp::log::assertion(reinterpret_cast<event*>(m_next) != std::addressof(m_event));

  if (m_next != nullptr) {
    m_next->m_prev = this;
  }
  m_event.m_handle->m_state = this;
}

inline auto event::awaiter::await_resume() noexcept -> void {
  if (std::exchange(m_suspended, false)) {
    // restore caller's async stack frame if it was suspended
    kphp::log::assertion(m_caller_async_stack_frame != nullptr);
    m_async_stack_root.top_async_stack_frame = std::exchange(m_caller_async_stack_frame, nullptr);
  }
}

inline auto event::set() noexcept -> void {
  void* prev_value{std::exchange(m_handle->m_state, this)};
  if (prev_value == this || prev_value == nullptr) [[unlikely]] {
    return;
  }
  auto* awaiter{static_cast<event::awaiter*>(prev_value)};
  while (awaiter != nullptr) {
    auto* next{awaiter->m_next};
    awaiter->m_awaiting_coroutine.resume();
    awaiter = next;
  }
}

inline auto event::unset() noexcept -> void {
  if (m_handle->m_state == this) {
    m_handle->m_state = nullptr;
  }
}

inline auto event::is_set() const noexcept -> bool {
  return m_handle->m_state == this;
}

inline auto event::operator co_await() noexcept {
  return event::awaiter{*this};
}

} // namespace kphp::coro
