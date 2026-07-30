// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <coroutine>
#include <memory>
#include <utility>
#include <variant>

#include "common/mixin/not_copyable.h"
#include "common/wrappers/overloaded.h"
#include "runtime-common/core/allocator/script-allocator-managed.h"
#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/coroutine/coroutine-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::coro {

class event {
  using awaiter_node = kphp::stl::intrusive::list_node<std::coroutine_handle<>>;
  using awaiters_list = kphp::stl::intrusive::list<awaiter_node>;

  struct event_controller;

  struct awaiter {
    awaiter_node m_awaiting_coroutine_node;
    bool m_suspended{};
    event_controller& m_controller;
    kphp::coro::async_stack_root& m_async_stack_root;
    kphp::coro::async_stack_frame* m_caller_async_stack_frame{};

    explicit awaiter(event_controller& event_controller) noexcept
        : m_controller(event_controller),
          m_async_stack_root(CoroutineInstanceState::get().coroutine_stack_root) {}

    awaiter(const awaiter&) = delete;
    awaiter(awaiter&&) = delete;
    auto operator=(const awaiter&) -> awaiter& = delete;
    auto operator=(awaiter&&) -> awaiter& = delete;
    ~awaiter() = default;

    auto await_ready() const noexcept -> bool;
    template<std::derived_from<kphp::coro::async_stack_element> caller_promise_type>
    auto await_suspend(std::coroutine_handle<caller_promise_type> awaiting_coroutine) noexcept -> void;
    auto await_resume() noexcept -> void;
  };

  struct event_controller : kphp::memory::script_allocator_managed, vk::not_copyable {
    // 1) std::monostate => not set and no coroutines are waiting
    // 2) non empty list => linked list of coroutines waiting for the event to trigger
    // 3) empty list => The event is triggered and all coroutines are resumed
    std::variant<std::monostate, awaiters_list> m_state;

    auto set() noexcept -> void;
    auto unset() noexcept -> void;
    auto is_set() const noexcept -> bool;
  };

  std::unique_ptr<event_controller> m_controller;

public:
  event() noexcept
      : m_controller(std::make_unique<event_controller>()) {
    kphp::log::assertion(m_controller != nullptr);
  }

  event(event&& other) noexcept
      : m_controller(std::move(other.m_controller)) {}

  event& operator=(event&& other) noexcept {
    if (this != std::addressof(other)) {
      m_controller = std::move(other.m_controller);
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

inline auto event::awaiter::await_ready() const noexcept -> bool {
  return m_controller.is_set();
}

template<std::derived_from<kphp::coro::async_stack_element> caller_promise_type>
auto event::awaiter::await_suspend(std::coroutine_handle<caller_promise_type> awaiting_coroutine) noexcept -> void {
  // save caller's async stack frame
  m_caller_async_stack_frame = m_async_stack_root.top_async_stack_frame;

  m_suspended = true;
  m_awaiting_coroutine_node = kphp::stl::intrusive::make_list_node<std::coroutine_handle<>>(awaiting_coroutine);

  // possibly this assertion is unnecessary, but it is left, because it was there earlier
  kphp::log::assertion(!m_controller.is_set());

  if (std::holds_alternative<std::monostate>(m_controller.m_state)) {
    m_controller.m_state = awaiters_list{};
  }

  std::get<awaiters_list>(m_controller.m_state).push_front(m_awaiting_coroutine_node);
}

inline auto event::awaiter::await_resume() noexcept -> void {
  if (std::exchange(m_suspended, false)) {
    // restore caller's async stack frame if it was suspended
    kphp::log::assertion(m_caller_async_stack_frame != nullptr);
    m_async_stack_root.top_async_stack_frame = std::exchange(m_caller_async_stack_frame, nullptr);
  }
}

inline auto event::event_controller::set() noexcept -> void {
  if (std::holds_alternative<std::monostate>(m_state)) {
    m_state = awaiters_list{};
    return;
  }

  awaiters_list awaiters;
  awaiters.splice(awaiters.begin(), std::get<awaiters_list>(m_state));
  while (!awaiters.empty()) {
    auto& coroutine{awaiters.front()};
    /*
     * We can remove pop_front() here, because list node will be destroyed after resume and in its
     * destructor will call unlink() [1]. But we left pop_front() for better readability and safety
     * (in the future invariant [1] may not work).
     */
    awaiters.pop_front();
    coroutine.resume();
  }
}

inline auto event::event_controller::unset() noexcept -> void {
  if (is_set()) {
    m_state = std::monostate{};
  }
}

inline auto event::event_controller::is_set() const noexcept -> bool {
  return std::visit(overloaded([](std::monostate) noexcept { return false; }, [](const awaiters_list& list) noexcept { return list.empty(); }), m_state);
}

inline auto event::set() noexcept -> void {
  kphp::log::assertion(m_controller != nullptr);
  m_controller->set();
}

inline auto event::unset() noexcept -> void {
  kphp::log::assertion(m_controller != nullptr);
  m_controller->unset();
}

inline auto event::is_set() const noexcept -> bool {
  kphp::log::assertion(m_controller != nullptr);
  return m_controller->is_set();
}

inline auto event::operator co_await() noexcept {
  kphp::log::assertion(m_controller != nullptr);
  return event::awaiter{*this->m_controller};
}

} // namespace kphp::coro
