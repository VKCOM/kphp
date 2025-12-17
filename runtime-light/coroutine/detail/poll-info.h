// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <coroutine>
#include <cstdint>
#include <optional>
#include <utility>
#include <variant>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/coroutine/poll.h"
#include "runtime-light/k2-platform/k2-api.h"

namespace kphp::coro::detail {

struct poll_info {
  enum class scheduled_coroutine_status : uint8_t { scheduled, cancelled };
  using timed_events = kphp::stl::multimap<k2::TimePoint, detail::poll_info&, kphp::memory::script_allocator>;
  using parked_polls = kphp::stl::multimap<k2::descriptor, detail::poll_info&, kphp::memory::script_allocator>;
  using scheduled_coroutines = kphp::stl::list<std::pair<std::coroutine_handle<>, scheduled_coroutine_status>, kphp::memory::script_allocator>;
  using schedule_position_type = std::variant<std::monostate, scheduled_coroutines::iterator, parked_polls::iterator>;

  k2::descriptor m_descriptor{k2::INVALID_PLATFORM_DESCRIPTOR};
  std::coroutine_handle<> m_awaiting_coroutine;
  std::optional<timed_events::iterator> m_timer_pos;
  schedule_position_type m_schedule_pos;

  kphp::coro::poll_status m_poll_status{kphp::coro::poll_status::error};
  kphp::coro::poll_op m_poll_op;

  poll_info(k2::descriptor descriptor, kphp::coro::poll_op poll_op) noexcept
      : m_descriptor(descriptor),
        m_poll_op(poll_op) {}

  ~poll_info() = default;

  poll_info(const detail::poll_info&) = delete;
  poll_info(detail::poll_info&&) = delete;
  poll_info& operator=(const detail::poll_info&) = delete;
  poll_info& operator=(detail::poll_info&&) = delete;

  auto operator co_await() noexcept {
    struct poll_awaiter {
      detail::poll_info& m_poll_info;

      explicit poll_awaiter(detail::poll_info& poll_info) noexcept
          : m_poll_info(poll_info) {}

      constexpr auto await_ready() const noexcept -> bool {
        return false;
      }

      auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> void {
        m_poll_info.m_awaiting_coroutine = awaiting_coroutine;
      }

      auto await_resume() const noexcept -> kphp::coro::poll_status {
        return m_poll_info.m_poll_status;
      }
    };
    return poll_awaiter{*this};
  }
};

} // namespace kphp::coro::detail
