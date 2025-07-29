// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <coroutine>
#include <optional>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/coroutine/poll.h"
#include "runtime-light/k2-platform/k2-api.h"

namespace kphp::coro::detail {

struct poll_info {
  using timed_events = kphp::stl::multimap<k2::TimePoint, detail::poll_info&, kphp::memory::script_allocator>;
  using awaiting_polls = kphp::stl::multimap<k2::descriptor, detail::poll_info&, kphp::memory::script_allocator>;

  k2::descriptor m_descriptor{k2::INVALID_PLATFORM_DESCRIPTOR};
  std::coroutine_handle<> m_awaiting_coroutine;
  kphp::coro::poll_op m_poll_op;

  std::optional<timed_events::iterator> m_timer_pos;
  std::optional<awaiting_polls::iterator> m_awaiting_pos;

  bool m_processed{};
  kphp::coro::poll_status m_poll_status{kphp::coro::poll_status::error};

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
