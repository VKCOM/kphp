// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <type_traits>
#include <utility>
#include <variant>

#include "common/containers/final_action.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/coroutine/async-stack-methods.h"
#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/coroutine/concepts.h"
#include "runtime-light/coroutine/coroutine-state.h"
#include "runtime-light/coroutine/detail/poll-info.h"
#include "runtime-light/coroutine/detail/task-self-deleting.h"
#include "runtime-light/coroutine/detail/timer-handle.h"
#include "runtime-light/coroutine/poll.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/coroutine/type-traits.h"
#include "runtime-light/coroutine/when-any.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::coro {

enum class timeout_status : uint8_t {
  timeout,
};

class io_scheduler {
  static constexpr auto MAX_EVENTS_TO_PROCESS = static_cast<size_t>(16);

  kphp::coro::detail::timer_handle m_timer_handle;
  kphp::coro::detail::poll_info::timed_events m_timed_events;
  kphp::stl::vector<k2::descriptor, kphp::memory::script_allocator> m_accepted_descriptors;

  kphp::coro::detail::poll_info::parked_polls m_parked_polls;
  kphp::stl::vector<std::coroutine_handle<>, kphp::memory::script_allocator> m_scheduled_tasks;
  kphp::stl::vector<std::coroutine_handle<>, kphp::memory::script_allocator> m_scheduled_tasks_tmp;

  CoroutineInstanceState& m_coroutine_instance_state{CoroutineInstanceState::get()};

  auto make_cancellation_handler(kphp::coro::detail::poll_info& poll_info) noexcept;
  template<typename rep_type, typename period_type>
  auto make_timeout_task(std::chrono::duration<rep_type, period_type> timeout) noexcept -> kphp::coro::task<timeout_status>;

  auto process_timeout() noexcept -> void;
  auto process_update(k2::descriptor descriptor) noexcept -> void;
  auto process_accept(k2::descriptor descriptor) noexcept -> void;

  auto update_timer() noexcept -> void;
  [[nodiscard]] auto add_timer_token(k2::TimePoint time_point,
                                     kphp::coro::detail::poll_info& poll_info) noexcept -> kphp::coro::detail::poll_info::timed_events::iterator;
  template<typename rep_type, typename period_type>
  [[nodiscard]] auto add_timer_token(std::chrono::duration<rep_type, period_type> timeout,
                                     kphp::coro::detail::poll_info& poll_info) noexcept -> kphp::coro::detail::poll_info::timed_events::iterator;
  auto remove_timer_token(kphp::coro::detail::poll_info::timed_events::iterator pos) noexcept -> void;

public:
  io_scheduler() noexcept {
    m_scheduled_tasks.reserve(MAX_EVENTS_TO_PROCESS);
    m_scheduled_tasks_tmp.reserve(MAX_EVENTS_TO_PROCESS);
  }

  ~io_scheduler() = default;

  io_scheduler(const io_scheduler&) = delete;
  io_scheduler(io_scheduler&&) = delete;
  io_scheduler& operator=(const io_scheduler&) = delete;
  io_scheduler& operator=(io_scheduler&&) = delete;

  static auto get() noexcept -> io_scheduler&;

  /**
   * @return True if the task queue is empty and zero tasks are scheduled for execution.
   */
  auto empty() const noexcept -> bool;

  /**
   * @brief Processes pending I/O events and scheduled tasks.
   * @return k2::PollStatus indicating whether tasks were processed or if polling should continue.
   */
  auto process_events() noexcept -> k2::PollStatus;

  /**
   * @brief Spawns a new coroutine to be executed by the scheduler.
   * @param coroutine The coroutine to spawn.
   * @return True if the coroutine was successfully spawned, false otherwise.
   */
  template<kphp::coro::concepts::coroutine coroutine_type>
  auto spawn(coroutine_type coroutine) noexcept -> bool;

  /**
   * @brief Spawns a new coroutine that immediately starts the passed coroutine.
   * @param corotuine The coroutine to start.
   * @return True if the coroutine was successfully started, false otherwise.
   */
  template<kphp::coro::concepts::coroutine coroutine_type>
  auto start(coroutine_type coroutine) noexcept -> bool;

  /**
   * @brief Schedules the current coroutine to be resumed later.
   * @return A task that needs to be co_awaited to schedule the coroutine.
   */
  [[nodiscard]] auto schedule() noexcept;
  /**
   * @brief Schedules a coroutine for execution.
   * @param coroutine The coroutine to schedule.
   * @return A task that will yield the coroutine's return value when completed.
   */
  template<kphp::coro::concepts::coroutine coroutine_type>
  [[nodiscard]] auto schedule(coroutine_type coroutine) noexcept -> kphp::coro::task<typename kphp::coro::coroutine_traits<coroutine_type>::return_type>;
  /**
   * @brief Schedules a coroutine with a timeout.
   * @param coroutine The coroutine to schedule.
   * @param timeout Maximum duration to wait for the coroutine to complete. Given zero or negative timeout this behaves identical to schedule(coroutine).
   * @return A task that yields either the coroutine's result or a timeout status.
   */
  template<kphp::coro::concepts::coroutine coroutine_type, typename rep_type, typename period_type>
  [[nodiscard]] auto schedule(coroutine_type coroutine, std::chrono::duration<rep_type, period_type> timeout) noexcept
      -> kphp::coro::task<std::expected<typename kphp::coro::coroutine_traits<coroutine_type>::return_type, timeout_status>>;
  /**
   * @brief Schedules a delay before continuing execution.
   * @param amount Duration to wait before resuming. Given zero or negative amount of time this behaves identical to schedule().
   * @return A task that completes after the specified duration.
   */
  template<typename rep_type, typename period_type>
  [[nodiscard]] auto schedule_after(std::chrono::duration<rep_type, period_type> amount) noexcept -> kphp::coro::task<>;

  /**
   * @brief Yields execution to allow other tasks to run.
   * @return A task that represents the yield operation.
   */
  [[nodiscard]] auto yield() noexcept;
  /**
   * @brief Yields execution for a specified duration.
   * @param amount Duration to yield for. Given zero or negative amount of time this behaves identical to yield().
   * @return A task that completes after the specified duration.
   */
  template<typename rep_type, typename period_type>
  [[nodiscard]] auto yield_for(std::chrono::duration<rep_type, period_type> amount) noexcept -> kphp::coro::task<>;

  /**
   * @brief Polls a descriptor for I/O events.
   * @param descriptor The descriptor to poll.
   * @param poll_op The operation to poll for (read/write).
   * @param timeout Maximum duration to wait for the event (0 means no timeout).
   * @return A task that yields the poll status when completed.
   */
  [[nodiscard]] auto poll(k2::descriptor descriptor, kphp::coro::poll_op poll_op,
                          std::chrono::nanoseconds timeout = std::chrono::nanoseconds{0}) noexcept -> kphp::coro::task<poll_status>;

  /**
   * @brief Accepts an incoming connection with optional timeout.
   * @param timeout Maximum duration to wait for a connection (0 means no timeout).
   * @return A task that yields the new connection descriptor when completed or k2::INVALID_PLATFORM_DESCRIPTOR on timeout.
   */
  [[nodiscard]] auto accept(std::chrono::nanoseconds timeout = std::chrono::nanoseconds{0}) noexcept -> kphp::coro::task<k2::descriptor>;
};

// ================================================================================================

inline auto io_scheduler::make_cancellation_handler(kphp::coro::detail::poll_info& poll_info) noexcept {
  return vk::final_action{[this, &poll_info] noexcept {
    if (poll_info.m_parking_pos) [[unlikely]] {
      m_parked_polls.erase(*std::exchange(poll_info.m_parking_pos, std::nullopt));
    }
    if (poll_info.m_timer_pos) [[unlikely]] {
      remove_timer_token(*std::exchange(poll_info.m_timer_pos, std::nullopt));
    }
  }};
}

template<typename rep_type, typename period_type>
auto io_scheduler::make_timeout_task(std::chrono::duration<rep_type, period_type> timeout) noexcept -> kphp::coro::task<timeout_status> {
  co_await schedule_after(timeout);
  co_return timeout_status::timeout;
}

inline auto io_scheduler::process_timeout() noexcept -> void {
  k2::TimePoint now{};
  k2::instant(std::addressof(now));
  // process all timed events that have reached their timeout
  while (!m_timed_events.empty()) {
    auto first{m_timed_events.begin()};
    auto& [time_point, poll_info]{*first};
    // verify that the poll_info corresponds to the current timed event
    kphp::log::assertion(poll_info.m_timer_pos.has_value() && *poll_info.m_timer_pos == first);
    // if the current timed event's time point is in the future, stop processing
    if (time_point.time_point_ns > now.time_point_ns) {
      break;
    }
    // ensure that this poll has not been processed yet
    kphp::log::assertion(!std::exchange(poll_info.m_processed, true));

    remove_timer_token(*std::exchange(poll_info.m_timer_pos, std::nullopt));
    // timeout's occured, so we can safely remove the awaiting task from the awaiting tasks list
    if (poll_info.m_parking_pos.has_value()) {
      m_parked_polls.erase(*std::exchange(poll_info.m_parking_pos, std::nullopt));
    }
    // mark the poll status as a timeout and schedule the awaiting coroutine for execution
    poll_info.m_poll_status = kphp::coro::poll_status::timeout;
    m_scheduled_tasks.emplace_back(poll_info.m_awaiting_coroutine);
  }

  update_timer();
}

inline auto io_scheduler::process_update(k2::descriptor descriptor) noexcept -> void {
  if (descriptor == m_timer_handle.descriptor()) [[unlikely]] {
    return process_timeout();
  }

  k2::StreamStatus stream_status{};
  k2::stream_status(descriptor, std::addressof(stream_status));
  if (stream_status.libc_errno != k2::errno_ok) [[unlikely]] {
    return kphp::log::warning("error retrieving stream status: error code -> {}, stream descriptor -> {}", stream_status.libc_errno, descriptor);
  }

  auto [first, last]{m_parked_polls.equal_range(descriptor)};
  if (first == last) { // no tasks waiting for updates on this descriptor
    return;
  }

  static constexpr auto complete_poll_on_update{
      [](io_scheduler& io_scheduler, kphp::coro::detail::poll_info& poll_info, kphp::coro::poll_status poll_status) noexcept {
        // ensure that this poll has not been processed yet
        kphp::log::assertion(!std::exchange(poll_info.m_processed, true));
        io_scheduler.m_parked_polls.erase(*std::exchange(poll_info.m_parking_pos, std::nullopt));
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
    kphp::log::assertion(poll_info.m_parking_pos.has_value() && *poll_info.m_parking_pos == std::exchange(first, std::next(first)));
    switch (poll_info.m_poll_op) {
    case kphp::coro::poll_op::read:
      switch (stream_status.read_status) {
      case k2::IOStatus::IOAvailable:
        std::invoke(complete_poll_on_update, *this, poll_info, kphp::coro::poll_status::event);
        break;

      case k2::IOStatus::IOClosed:
        std::invoke(complete_poll_on_update, *this, poll_info, kphp::coro::poll_status::closed);
        break;

      case k2::IOStatus::IOBlocked:
        break;
      }
      break;
    case kphp::coro::poll_op::write:
      if (static_cast<bool>(stream_status.please_shutdown_write)) {
        std::invoke(complete_poll_on_update, *this, poll_info, kphp::coro::poll_status::closed);
        break;
      }

      switch (stream_status.write_status) {
      case k2::IOStatus::IOAvailable:
        std::invoke(complete_poll_on_update, *this, poll_info, kphp::coro::poll_status::event);
        break;

      case k2::IOStatus::IOClosed:
        std::invoke(complete_poll_on_update, *this, poll_info, kphp::coro::poll_status::closed);
        break;

      case k2::IOStatus::IOBlocked:
        break;
      }
      break;
    }
  }
}

inline auto io_scheduler::process_accept(k2::descriptor descriptor) noexcept -> void {
  const auto pos{m_parked_polls.find(k2::INVALID_PLATFORM_DESCRIPTOR)};
  if (pos == m_parked_polls.end()) { // no one is waiting for new connection
    m_accepted_descriptors.emplace_back(descriptor);
    return;
  }

  auto& [_, poll_info]{*pos};
  // verify that the poll_info corresponds to the new connect event
  kphp::log::assertion(poll_info.m_descriptor == k2::INVALID_PLATFORM_DESCRIPTOR && poll_info.m_parking_pos.has_value() && *poll_info.m_parking_pos == pos);
  // ensure that this poll has not been processed yet
  kphp::log::assertion(!std::exchange(poll_info.m_processed, true));
  m_parked_polls.erase(*std::exchange(poll_info.m_parking_pos, std::nullopt));
  // if a timer was set for this poll, remove it as the poll is now complete
  if (poll_info.m_timer_pos.has_value()) {
    remove_timer_token(*std::exchange(poll_info.m_timer_pos, std::nullopt));
  }
  // update the poll status and schedule the awaiting coroutine for execution
  poll_info.m_descriptor = descriptor;
  poll_info.m_poll_status = kphp::coro::poll_status::event;
  m_scheduled_tasks.emplace_back(poll_info.m_awaiting_coroutine);
}

inline auto io_scheduler::update_timer() noexcept -> void {
  // if the list of timed events is empty, clear the timer handle as there are no timeouts to manage
  if (m_timed_events.empty()) {
    m_timer_handle.clear();
    return;
  }

  auto& [time_point, _]{*m_timed_events.begin()};
  if (!m_timer_handle.reset(time_point)) [[unlikely]] {
    kphp::log::warning("error resetting timer handle, unable to set time point -> {}, current time point -> {}", time_point.time_point_ns,
                       m_timer_handle.time_point().time_point_ns);
  }
}

inline auto io_scheduler::add_timer_token(k2::TimePoint time_point,
                                          kphp::coro::detail::poll_info& poll_info) noexcept -> kphp::coro::detail::poll_info::timed_events::iterator {
  const auto pos{m_timed_events.emplace(time_point, poll_info)};
  if (pos == m_timed_events.begin()) {
    update_timer();
  }
  return pos;
}

template<typename rep_type, typename period_type>
auto io_scheduler::add_timer_token(std::chrono::duration<rep_type, period_type> timeout,
                                   kphp::coro::detail::poll_info& poll_info) noexcept -> kphp::coro::detail::poll_info::timed_events::iterator {
  k2::TimePoint time_point{};
  k2::instant(std::addressof(time_point));
  time_point.time_point_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(timeout).count();
  return add_timer_token(time_point, poll_info);
}

inline auto io_scheduler::remove_timer_token(kphp::coro::detail::poll_info::timed_events::iterator pos) noexcept -> void {
  const bool is_first{pos == m_timed_events.begin()};
  m_timed_events.erase(pos);
  if (is_first) {
    update_timer();
  }
}

inline auto io_scheduler::empty() const noexcept -> bool {
  return m_scheduled_tasks.empty() && m_parked_polls.empty() && m_timed_events.empty();
}

inline auto io_scheduler::process_events() noexcept -> k2::PollStatus {
  // process events as long as the scheduler is allowed to work and there is work to do
  for (bool keep_working{true}; keep_working; keep_working = !static_cast<bool>(k2::control_flags()->please_yield.load())) {
    size_t num_events{};
    std::array<std::pair<k2::descriptor, k2::UpdateStatus>, MAX_EVENTS_TO_PROCESS> recent_events{};
    for (; num_events < recent_events.size(); ++num_events) {
      auto descriptor{k2::INVALID_PLATFORM_DESCRIPTOR};
      auto event_kind{k2::take_update(std::addressof(descriptor))};
      if (event_kind == k2::UpdateStatus::NoUpdates) {
        break;
      }
      recent_events[num_events] = {descriptor, event_kind};
    }

    for (const auto& [descriptor, event_kind] : recent_events | std::views::take(num_events)) {
      switch (event_kind) {
      case k2::UpdateStatus::UpdateExisted:
        process_update(descriptor);
        break;
      case k2::UpdateStatus::NewDescriptor:
        process_accept(descriptor);
        break;
      case k2::UpdateStatus::NoUpdates:
        [[fallthrough]];
      default:
        kphp::log::assertion(false);
      }
    }

    if (m_scheduled_tasks.empty()) {
      return empty() ? k2::PollStatus::PollFinishedOk : k2::PollStatus::PollBlocked;
    }

    kphp::log::assertion(m_scheduled_tasks_tmp.empty());
    std::swap(m_scheduled_tasks, m_scheduled_tasks_tmp);
    std::ranges::for_each(m_scheduled_tasks_tmp, [&coroutine_next_stack_root = m_coroutine_instance_state.next_root](const auto& coroutine) noexcept {
      kphp::coro::resume(coroutine, coroutine_next_stack_root);
    });
    m_scheduled_tasks_tmp.clear();
  }

  return empty() ? k2::PollStatus::PollFinishedOk : k2::PollStatus::PollReschedule;
}

template<kphp::coro::concepts::coroutine coroutine_type>
auto io_scheduler::spawn(coroutine_type coroutine) noexcept -> bool {
  auto owned_task{kphp::coro::detail::make_task_self_deleting(std::move(coroutine))};
  auto handle{owned_task.get_handle()};
  if (!handle || handle.done()) [[unlikely]] {
    return false;
  }
  m_scheduled_tasks.emplace_back(handle);
  return true;
}

template<kphp::coro::concepts::coroutine coroutine_type>
auto io_scheduler::start(coroutine_type coroutine) noexcept -> bool {
  auto owned_task{kphp::coro::detail::make_task_self_deleting(std::move(coroutine))};
  auto handle{owned_task.get_handle()};
  if (!handle || handle.done()) [[unlikely]] {
    return false;
  }
  kphp::coro::resume(handle, m_coroutine_instance_state.next_root);
  return true;
}

inline auto io_scheduler::schedule() noexcept {
  class schedule_operation {
    friend class io_scheduler;
    io_scheduler& m_scheduler;
    kphp::coro::async_stack_frame* const m_async_stack_frame{};

    explicit schedule_operation(io_scheduler& scheduler) noexcept
        : m_scheduler(scheduler),
          m_async_stack_frame(m_scheduler.m_coroutine_instance_state.next_root->next_async_stack_root->top_async_stack_frame) {}

  public:
    constexpr auto await_ready() const noexcept -> bool {
      return false;
    }

    auto await_suspend(std::coroutine_handle<> coro) noexcept -> void {
      m_scheduler.m_scheduled_tasks.emplace_back(coro);
    }

    auto await_resume() const noexcept -> void {
      m_scheduler.m_coroutine_instance_state.next_root->next_async_stack_root->top_async_stack_frame = m_async_stack_frame;
    }
  };
  return schedule_operation{*this};
}

template<kphp::coro::concepts::coroutine coroutine_type>
auto io_scheduler::schedule(coroutine_type coroutine) noexcept -> kphp::coro::task<typename kphp::coro::coroutine_traits<coroutine_type>::return_type> {
  co_await schedule();
  co_return co_await std::move(coroutine);
}

template<kphp::coro::concepts::coroutine coroutine_type, typename rep_type, typename period_type>
auto io_scheduler::schedule(coroutine_type coroutine, std::chrono::duration<rep_type, period_type> timeout) noexcept
    -> kphp::coro::task<std::expected<typename kphp::coro::coroutine_traits<coroutine_type>::return_type, timeout_status>> {
  using expected_return_type = typename kphp::coro::coroutine_traits<coroutine_type>::return_type;

  if (timeout <= std::chrono::duration<rep_type, period_type>{0}) [[unlikely]] {
    if constexpr (std::is_void_v<expected_return_type>) {
      co_await schedule(std::move(coroutine));
      co_return std::expected<expected_return_type, timeout_status>{};
    } else {
      co_return std::expected<expected_return_type, timeout_status>{co_await schedule(std::move(coroutine))};
    }
  }

  auto result{co_await kphp::coro::when_any(std::move(coroutine), make_timeout_task(timeout))};
  if (std::holds_alternative<timeout_status>(result)) [[unlikely]] {
    co_return std::unexpected{std::move(std::get<1>(result))};
  }

  if constexpr (std::is_void_v<expected_return_type>) {
    co_return std::expected<expected_return_type, timeout_status>{};
  } else {
    co_return std::expected<expected_return_type, timeout_status>{std::move(std::get<0>(result))};
  }
}

template<typename rep_type, typename period_type>
auto io_scheduler::schedule_after(std::chrono::duration<rep_type, period_type> amount) noexcept -> kphp::coro::task<> {
  if (amount <= std::chrono::duration<rep_type, period_type>{0}) [[unlikely]] {
    co_return co_await schedule();
  }

  // poll_op is not actually used here
  kphp::coro::detail::poll_info poll_info{k2::INVALID_PLATFORM_DESCRIPTOR, kphp::coro::poll_op::read};
  const auto cancellation_handler{make_cancellation_handler(poll_info)};
  poll_info.m_timer_pos = add_timer_token(amount, poll_info);
  co_await poll_info;
}

inline auto io_scheduler::yield() noexcept {
  return schedule();
}

template<typename rep_type, typename period_type>
auto io_scheduler::yield_for(std::chrono::duration<rep_type, period_type> amount) noexcept -> kphp::coro::task<> {
  co_await schedule_after(amount);
}

inline auto io_scheduler::poll(k2::descriptor descriptor, kphp::coro::poll_op poll_op,
                               std::chrono::nanoseconds timeout) noexcept -> kphp::coro::task<poll_status> {
  k2::StreamStatus stream_status{};
  k2::stream_status(descriptor, std::addressof(stream_status));
  if (stream_status.libc_errno != k2::errno_ok) [[unlikely]] {
    kphp::log::warning("error retrieving stream status: error code -> {}, stream descriptor -> {}", stream_status.libc_errno, descriptor);
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
    if (static_cast<bool>(stream_status.please_shutdown_write)) {
      co_return kphp::coro::poll_status::closed;
    }

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

  using namespace std::chrono_literals;
  kphp::coro::detail::poll_info poll_info{descriptor, poll_op};
  const auto cancellation_handler{make_cancellation_handler(poll_info)};
  if (timeout > 0ns) {
    poll_info.m_timer_pos = add_timer_token(timeout, poll_info);
  }
  poll_info.m_parking_pos = m_parked_polls.emplace(poll_info.m_descriptor, poll_info);
  co_return co_await poll_info;
}

inline auto io_scheduler::accept(std::chrono::nanoseconds timeout) noexcept -> kphp::coro::task<k2::descriptor> {
  if (!m_accepted_descriptors.empty()) { // don't suspend if there is a connection
    const auto descriptor{m_accepted_descriptors.back()};
    m_accepted_descriptors.pop_back();
    co_return descriptor;
  }

  using namespace std::chrono_literals;
  // poll_op is not actually used here
  kphp::coro::detail::poll_info poll_info{k2::INVALID_PLATFORM_DESCRIPTOR, kphp::coro::poll_op::read};
  const auto cancellation_handler{make_cancellation_handler(poll_info)};
  if (timeout > 0ns) {
    poll_info.m_timer_pos = add_timer_token(timeout, poll_info);
  }
  poll_info.m_parking_pos = m_parked_polls.emplace(poll_info.m_descriptor, poll_info);
  co_await poll_info;
  co_return poll_info.m_descriptor;
}

} // namespace kphp::coro
