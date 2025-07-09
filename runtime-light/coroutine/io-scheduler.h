// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <coroutine>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <utility>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/coroutine/coroutine-state.h"
#include "runtime-light/coroutine/detail/poll-info.h"
#include "runtime-light/coroutine/detail/timer-handle.h"
#include "runtime-light/coroutine/poll.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/utils/logs.h"

namespace kphp::coro {

class io_scheduler {
  kphp::coro::detail::timer_handle m_timer_handle;
  kphp::coro::detail::poll_info::timed_events m_timed_events;
  kphp::stl::vector<k2::descriptor, kphp::memory::script_allocator> m_connections;
  CoroutineInstanceState& m_coro_instance_state{CoroutineInstanceState::get()};

  kphp::coro::detail::poll_info::awaiting_polls m_awaiting_polls;
  kphp::stl::vector<std::coroutine_handle<>, kphp::memory::script_allocator> m_scheduled_tasks;

  auto process_timeout() noexcept -> void;
  auto process_update(k2::descriptor descriptor) noexcept -> void;
  auto process_connect(k2::descriptor descriptor) noexcept -> void;

  [[nodiscard]] auto add_timer_token(k2::TimePoint time_point, kphp::coro::detail::poll_info& poll_info) noexcept
      -> kphp::coro::detail::poll_info::timed_events::iterator;
  template<typename rep_type, typename period_type>
  [[nodiscard]] auto add_timer_token(std::chrono::duration<rep_type, period_type> timeout, kphp::coro::detail::poll_info& poll_info) noexcept
      -> kphp::coro::detail::poll_info::timed_events::iterator;
  auto remove_timer_token(kphp::coro::detail::poll_info::timed_events::iterator pos) noexcept -> void;

  auto update_timeout() noexcept -> void;

public:
  io_scheduler() noexcept = default;
  ~io_scheduler() = default;

  io_scheduler(const io_scheduler&) = delete;
  io_scheduler(io_scheduler&&) = delete;
  io_scheduler& operator=(const io_scheduler&) = delete;
  io_scheduler& operator=(io_scheduler&&) = delete;

  static auto get() noexcept -> io_scheduler&;

  auto size() const noexcept -> size_t;

  auto process_events() noexcept -> void;

  [[nodiscard]] auto schedule() noexcept;

  template<typename return_type>
  [[nodiscard]] auto schedule(kphp::coro::task<return_type> task) noexcept -> kphp::coro::task<return_type>;

  [[nodiscard]] auto yield() noexcept;

  template<typename rep_type, typename period_type>
  [[nodiscard]] auto yield_for(std::chrono::duration<rep_type, period_type> amount) noexcept -> kphp::coro::task<>;

  template<typename rep_type, typename period_type>
  [[nodiscard]] auto poll(k2::descriptor descriptor, kphp::coro::poll_op poll_op, std::chrono::duration<rep_type, period_type> timeout = {0}) noexcept
      -> kphp::coro::task<poll_status>;

  template<typename rep_type, typename period_type>
  [[nodiscard]] auto connect(std::chrono::duration<rep_type, period_type> timeout = {0}) noexcept -> kphp::coro::task<k2::descriptor>;
};

// ================================================================================================

inline auto io_scheduler::process_timeout() noexcept -> void {
  // ensure that a valid timer handle exists when a timeout occurs
  kphp::log::assertion(m_timer_handle.handle() != k2::INVALID_PLATFORM_DESCRIPTOR);

  // process all timed events that have reached their timeout
  while (!m_timed_events.empty()) {
    auto first{m_timed_events.begin()};
    auto& [time_point, poll_info]{*first};
    // verify that the poll_info corresponds to the current timed event
    kphp::log::assertion(poll_info.m_timer_pos.has_value() && *poll_info.m_timer_pos == first);
    // if the current timed event's time point is in the future, stop processing
    if (time_point.time_point_ns > m_timer_handle.time_point().time_point_ns) {
      break;
    }
    // ensure that this poll has not been processed yet
    kphp::log::assertion(!std::exchange(poll_info.m_processed, true));

    m_timed_events.erase(*std::exchange(poll_info.m_timer_pos, std::nullopt));
    // timeout's occured, so we can safely remove the awaiting task from the awaiting tasks list
    if (poll_info.m_awaiting_pos.has_value()) {
      m_awaiting_polls.erase(*std::exchange(poll_info.m_awaiting_pos, std::nullopt));
    }
    // mark the poll status as a timeout and schedule the awaiting coroutine for execution
    poll_info.m_poll_status = kphp::coro::poll_status::timeout;
    m_scheduled_tasks.emplace_back(poll_info.m_awaiting_coroutine);
  }

  update_timeout();
}

inline auto io_scheduler::process_update(k2::descriptor descriptor) noexcept -> void {
  k2::StreamStatus stream_status{};
  k2::stream_status(descriptor, std::addressof(stream_status));
  if (stream_status.libc_errno != k2::errno_ok) [[unlikely]] {
    kphp::log::error("error retrieving stream status: error code -> {}, stream descriptor -> {}", stream_status.libc_errno, descriptor);
  }

  auto [first, last]{m_awaiting_polls.equal_range(descriptor)};
  if (first == last) { // no tasks waiting for updates on this descriptor
    return;
  }

  static constexpr auto complete_poll_on_update{
      [](io_scheduler& io_scheduler, kphp::coro::detail::poll_info& poll_info, kphp::coro::poll_status poll_status) noexcept {
        // ensure that this poll has not been processed yet
        kphp::log::assertion(!std::exchange(poll_info.m_processed, true));
        io_scheduler.m_awaiting_polls.erase(*std::exchange(poll_info.m_awaiting_pos, std::nullopt));
        // if a timer was set for this poll, remove it as the poll is now complete
        if (poll_info.m_timer_pos.has_value()) {
          io_scheduler.remove_timer_token(*std::exchange(poll_info.m_timer_pos, std::nullopt));
        }
        // update the poll status and schedule the awaiting coroutine for execution
        poll_info.m_poll_status = poll_status;
        io_scheduler.m_scheduled_tasks.emplace_back(poll_info.m_awaiting_coroutine);
      }};

  while (first != last) {
    auto& [_, poll_info]{*first};
    // verify that the poll_info corresponds to the current update event
    kphp::log::assertion(poll_info.m_awaiting_pos.has_value() && *poll_info.m_awaiting_pos == std::exchange(first, std::next(first)));
    switch (poll_info.m_poll_op) {
    case kphp::coro::poll_op::read: {
      switch (stream_status.read_status) {
      case k2::IOStatus::IOAvailable: {
        std::invoke(complete_poll_on_update, *this, poll_info, kphp::coro::poll_status::event);
        break;
      }
      case k2::IOStatus::IOClosed: {
        std::invoke(complete_poll_on_update, *this, poll_info, kphp::coro::poll_status::closed);
        break;
      }
      case k2::IOStatus::IOBlocked: {
        break;
      }
      }
    }
    case kphp::coro::poll_op::write: {
      switch (stream_status.write_status) {
      case k2::IOStatus::IOAvailable: {
        std::invoke(complete_poll_on_update, *this, poll_info, kphp::coro::poll_status::event);
        break;
      }
      case k2::IOStatus::IOClosed: {
        std::invoke(complete_poll_on_update, *this, poll_info, kphp::coro::poll_status::closed);
        break;
      }
      case k2::IOStatus::IOBlocked: {
        break;
      }
      }
    }
    }
  }
}

inline auto io_scheduler::process_connect(k2::descriptor descriptor) noexcept -> void {
  const auto pos{m_awaiting_polls.find(k2::INVALID_PLATFORM_DESCRIPTOR)};
  if (pos == m_awaiting_polls.end()) { // no one is waiting for new connection
    m_connections.emplace_back(descriptor);
    return;
  }

  auto& [_, poll_info]{*pos};
  // verify that the poll_info corresponds to the new connect event
  kphp::log::assertion(poll_info.m_descriptor == k2::INVALID_PLATFORM_DESCRIPTOR && poll_info.m_awaiting_pos.has_value() && *poll_info.m_awaiting_pos == pos);
  // ensure that this poll has not been processed yet
  kphp::log::assertion(!std::exchange(poll_info.m_processed, true));
  m_awaiting_polls.erase(*std::exchange(poll_info.m_awaiting_pos, std::nullopt));
  // if a timer was set for this poll, remove it as the poll is now complete
  if (poll_info.m_timer_pos.has_value()) {
    remove_timer_token(*std::exchange(poll_info.m_timer_pos, std::nullopt));
  }
  // update the poll status and schedule the awaiting coroutine for execution
  poll_info.m_descriptor = descriptor;
  poll_info.m_poll_status = kphp::coro::poll_status::event;
  m_scheduled_tasks.emplace_back(poll_info.m_awaiting_coroutine);
}

inline auto io_scheduler::update_timeout() noexcept -> void {
  // if the list of timed events is empty, clear the timer handle as there are no timeouts to manage
  if (m_timed_events.empty()) {
    m_timer_handle.clear();
    return;
  }

  auto& [time_point, _]{*m_timed_events.begin()};
  if (!m_timer_handle.reset(time_point)) [[unlikely]] {
    kphp::log::warning("failed to reset timer handle, unable to set time point -> {}, current time point -> {}", time_point.time_point_ns,
                       m_timer_handle.time_point().time_point_ns);
  }
}

inline auto io_scheduler::add_timer_token(k2::TimePoint time_point, kphp::coro::detail::poll_info& poll_info) noexcept
    -> kphp::coro::detail::poll_info::timed_events::iterator {
  const auto pos{m_timed_events.emplace(time_point, poll_info)};
  if (pos == m_timed_events.begin()) {
    update_timeout();
  }
  return pos;
}

template<typename rep_type, typename period_type>
auto io_scheduler::add_timer_token(std::chrono::duration<rep_type, period_type> timeout, kphp::coro::detail::poll_info& poll_info) noexcept
    -> kphp::coro::detail::poll_info::timed_events::iterator {
  k2::TimePoint time_point{};
  k2::instant(std::addressof(time_point));
  time_point.time_point_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(timeout).count();
  return add_timer_token(time_point, poll_info);
}

inline auto io_scheduler::remove_timer_token(kphp::coro::detail::poll_info::timed_events::iterator pos) noexcept -> void {
  const bool is_first{pos == m_timed_events.begin()};
  m_timed_events.erase(pos);
  if (is_first) {
    update_timeout();
  }
}

inline auto io_scheduler::size() const noexcept -> size_t {
  return m_scheduled_tasks.size() + m_awaiting_polls.size();
}

inline auto io_scheduler::process_events() noexcept -> void {
  for (;;) {
    k2::descriptor descriptor{k2::INVALID_PLATFORM_DESCRIPTOR};
    const auto event_kind{k2::take_update(std::addressof(descriptor))};
    switch (event_kind) {
    case k2::EventKind::StreamUpdate:
      [[likely]] {
        if (m_timer_handle.handle() != k2::INVALID_PLATFORM_DESCRIPTOR && m_timer_handle.handle() == descriptor) [[unlikely]] {
          process_timeout();
        } else {
          process_update(descriptor);
        }
        break;
      }
    case k2::EventKind::StreamConnect: {
      process_connect(descriptor);
      break;
    }
    case k2::EventKind::Nothing: {
      break;
    }
    }
  }
}

inline auto io_scheduler::schedule() noexcept {
  class schedule_operation {
    friend class io_scheduler;
    io_scheduler& m_scheduler;

    explicit schedule_operation(io_scheduler& scheduler) noexcept
        : m_scheduler(scheduler) {}

  public:
    constexpr auto await_ready() const noexcept -> bool {
      return false;
    }

    auto await_suspend(std::coroutine_handle<> coro) noexcept -> void {
      m_scheduler.m_scheduled_tasks.emplace_back(coro);
    }

    constexpr auto await_resume() const noexcept -> void {}
  };
  return schedule_operation{*this};
}

template<typename return_type>
auto io_scheduler::schedule(kphp::coro::task<return_type> task) noexcept -> kphp::coro::task<return_type> {
  co_await schedule();
  co_return co_await task;
}

inline auto io_scheduler::yield() noexcept {
  return schedule();
}

template<typename rep_type, typename period_type>
auto io_scheduler::yield_for(std::chrono::duration<rep_type, period_type> amount) noexcept -> kphp::coro::task<> {
  if (amount <= std::chrono::duration<rep_type, period_type>{0}) [[unlikely]] {
    co_return co_await schedule();
  }

  // poll_op is not actually used here
  kphp::coro::detail::poll_info poll_info{k2::INVALID_PLATFORM_DESCRIPTOR, kphp::coro::poll_op::read};
  poll_info.m_timer_pos = add_timer_token(amount, poll_info);
  co_await poll_info;
}

template<typename rep_type, typename period_type>
auto io_scheduler::poll(k2::descriptor descriptor, kphp::coro::poll_op poll_op, std::chrono::duration<rep_type, period_type> timeout) noexcept
    -> kphp::coro::task<poll_status> {
  k2::StreamStatus stream_status{};
  k2::stream_status(descriptor, std::addressof(stream_status));
  if (stream_status.libc_errno != k2::errno_ok) [[unlikely]] {
    co_return kphp::coro::poll_status::error;
  }

  switch (poll_op) {
  case kphp::coro::poll_op::read:
    switch (stream_status.read_status) {
    case k2::IOStatus::IOAvailable:
      co_return kphp::coro::poll_status::event;
    case k2::IOStatus::IOBlocked:
      break;
    case k2::IOStatus::IOClosed:
      co_return kphp::coro::poll_status::closed;
    }
    break;
  case kphp::coro::poll_op::write:
    switch (stream_status.write_status) {
    case k2::IOStatus::IOAvailable:
      co_return kphp::coro::poll_status::event;
    case k2::IOStatus::IOBlocked:
      break;
    case k2::IOStatus::IOClosed:
      co_return kphp::coro::poll_status::closed;
    }
    break;
  }

  kphp::coro::detail::poll_info poll_info{descriptor, poll_op};
  if (timeout > std::chrono::duration<rep_type, period_type>{0}) {
    poll_info.m_timer_pos = add_timer_token(timeout, poll_info);
  }
  poll_info.m_awaiting_pos = m_awaiting_polls.emplace(poll_info.m_descriptor, poll_info);
  co_return co_await poll_info;
}

template<typename rep_type, typename period_type>
auto io_scheduler::connect(std::chrono::duration<rep_type, period_type> timeout) noexcept -> kphp::coro::task<k2::descriptor> {
  if (!m_connections.empty()) { // don't suspend if there is a connection
    const auto descriptor{m_connections.back()};
    m_connections.pop_back();
    co_return descriptor;
  }

  // poll_op is not actually used here
  kphp::coro::detail::poll_info poll_info{k2::INVALID_PLATFORM_DESCRIPTOR, kphp::coro::poll_op::read};
  if (timeout > std::chrono::duration<rep_type, period_type>{0}) {
    poll_info.m_timer_pos = add_timer_token(timeout, poll_info);
  }
  poll_info.m_awaiting_pos = m_awaiting_polls.emplace(poll_info.m_descriptor, poll_info);
  co_await poll_info;
  co_return poll_info.m_descriptor;
}

} // namespace kphp::coro
