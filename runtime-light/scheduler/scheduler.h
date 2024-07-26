// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <coroutine>
#include <cstdint>
#include <variant>

#include "runtime-core/memory-resource/unsynchronized_pool_resource.h"

/**
 * Supported types of updates:
 * 1. NoEvent: there was not an update event, so it's up to scheduler what to do;
 * 2. IncomingStream(uint64_t): there is a new stream;
 * 3. UpdateOnStream(uint64_t): there is an update on some stream;
 * 4. UpdateOnTimer(uint64_t): there is an update event on timer;
 * 5. Yield: request to yield execution received.
 */
namespace ScheduleEvent {

struct NoEvent {};

struct Yield {};

struct IncomingStream {
  uint64_t stream_d{};
};

struct UpdateOnStream {
  uint64_t stream_d{};
};

struct UpdateOnTimer {
  uint64_t timer_d{};
};

using EventT = std::variant<NoEvent, IncomingStream, UpdateOnStream, UpdateOnTimer, Yield>;

} // namespace ScheduleEvent

enum class ScheduleStatus : uint8_t { Resumed, Skipped, Error };

/**
 * Supported types of awaitable events:
 * 1. Reschedule: yield execution, it's up to scheduler when the coroutine will continue its execution;
 * 2. IncomingStream: wait for incoming stream;
 * 3. UpdateOnStream(uint64_t): wait for update on specified stream;
 * 4. UpdateOnTimer(uint64_t): wait for update on specified timer.
 */
namespace WaitEvent {

struct Rechedule {};

struct IncomingStream {};

struct UpdateOnStream {
  uint64_t stream_d{};
};

struct UpdateOnTimer {
  uint64_t timer_d{};
};

using EventT = std::variant<Rechedule, IncomingStream, UpdateOnStream, UpdateOnTimer>;

}; // namespace WaitEvent

/**
 * Coroutine scheduler concept.
 *
 * Any type that is supposed to be used as a coroutine scheduler should conform to following interface:
 * 1. be constructible from `memory_resource::unsyncrhonized_pool_resource&`;
 * 2. have static `get` function that returns a reference to scheduler instance;
 * 3. have `schedule` method that takes an event and schedules coroutines for execution and returning `ScheduleResult`
 *    signaling the status of current `vk_k2_poll` call;
 * 4. have `wait_for_update` method that blocks a coroutine until update on specified stream happens;
 * 5. have `wait_for_incoming_stream` that blocks a coroutine until incoming stream is accepted;
 * 6. have 'wait_for_reschedule' method that blocks a coroutine for better times.
 */
template<class scheduler_t>
concept CoroutineSchedulerConcept =
  std::constructible_from<scheduler_t, memory_resource::unsynchronized_pool_resource &>
  && requires(scheduler_t && s, ScheduleEvent::EventT schedule_event, std::coroutine_handle<> coro, WaitEvent::EventT wait_event) {
  { scheduler_t::get() } noexcept -> std::same_as<scheduler_t &>;
  { s.done() } noexcept -> std::convertible_to<bool>;
  { s.schedule(schedule_event) } noexcept -> std::same_as<ScheduleStatus>;
  { s.suspend(coro, wait_event) } noexcept -> std::same_as<void>;
};

// === SimpleCoroutineScheduler ===================================================================

class SimpleCoroutineScheduler {
  template<typename Key, typename Value>
  using unordered_map = memory_resource::stl::unordered_map<Key, Value, memory_resource::unsynchronized_pool_resource>;

  template<typename Value>
  using deque = memory_resource::stl::deque<Value, memory_resource::unsynchronized_pool_resource>;

  deque<std::coroutine_handle<>> yield_coros;
  deque<std::coroutine_handle<>> awaiting_for_stream_coros;
  unordered_map<uint64_t, std::coroutine_handle<>> awaiting_for_update_coros;

  ScheduleStatus scheduleOnNoEvent() noexcept;
  ScheduleStatus scheduleOnIncomingStream() noexcept;
  ScheduleStatus scheduleOnStreamUpdate(uint64_t) noexcept;
  ScheduleStatus scheduleOnYield() noexcept;

public:
  explicit SimpleCoroutineScheduler(memory_resource::unsynchronized_pool_resource &) noexcept;

  static SimpleCoroutineScheduler &get() noexcept;

  bool done() const noexcept;

  ScheduleStatus schedule(ScheduleEvent::EventT) noexcept;

  void suspend(std::coroutine_handle<>, WaitEvent::EventT) noexcept;
};
