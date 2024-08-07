// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>
#include <variant>

#include "runtime-core/memory-resource/resource_allocator.h"
#include "runtime-core/memory-resource/unsynchronized_pool_resource.h"
#include "runtime-core/utils/hash.h"
#include "runtime-light/utils/concepts.h"

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

struct Rechedule {
  static constexpr size_t HASH_VALUE = 4001;
  bool operator==([[maybe_unused]] const Rechedule &other) const noexcept {
    return true;
  }
};

struct IncomingStream {
  static constexpr size_t HASH_VALUE = 4003;
  bool operator==([[maybe_unused]] const IncomingStream &other) const noexcept {
    return true;
  }
};

struct UpdateOnStream {
  uint64_t stream_d{};

  bool operator==(const UpdateOnStream &other) const noexcept {
    return stream_d == other.stream_d;
  }
};

struct UpdateOnTimer {
  uint64_t timer_d{};

  bool operator==(const UpdateOnTimer &other) const noexcept {
    return timer_d == other.timer_d;
  }
};

using EventT = std::variant<Rechedule, IncomingStream, UpdateOnStream, UpdateOnTimer>;

}; // namespace WaitEvent

// std::hash specializations for WaitEvent types

template<>
struct std::hash<WaitEvent::Rechedule> {
  size_t operator()([[maybe_unused]] const WaitEvent::Rechedule &v) const noexcept {
    return WaitEvent::Rechedule::HASH_VALUE;
  }
};

template<>
struct std::hash<WaitEvent::IncomingStream> {
  size_t operator()([[maybe_unused]] const WaitEvent::IncomingStream &v) const noexcept {
    return WaitEvent::IncomingStream::HASH_VALUE;
  }
};

template<>
struct std::hash<WaitEvent::UpdateOnStream> {
  size_t operator()(const WaitEvent::UpdateOnStream &v) const noexcept {
    return v.stream_d;
  }
};

template<>
struct std::hash<WaitEvent::UpdateOnTimer> {
  size_t operator()(const WaitEvent::UpdateOnTimer &v) const noexcept {
    return v.timer_d;
  }
};

/**
 * SuspendToken type that binds an event and a coroutine waiting for that event.
 */
using SuspendToken = std::pair<std::coroutine_handle<>, WaitEvent::EventT>;

template<>
struct std::hash<SuspendToken> {
  size_t operator()(const SuspendToken &token) const noexcept {
    size_t suspend_token_hash{std::hash<std::coroutine_handle<>>{}(token.first)};
    hash_combine(suspend_token_hash, token.second);
    return suspend_token_hash;
  }
};

/**
 * Coroutine scheduler concept.
 *
 * Any type that is supposed to be used as a coroutine scheduler should conform to following interface:
 * 1. be constructible from `memory_resource::unsyncrhonized_pool_resource&`;
 * 2. have static `get` function that returns a reference to scheduler instance;
 * 3. have `done` method that returns whether scheduler's scheduled all coroutines;
 * 4. have `schedule` method that takes an event and schedules coroutines for execution;
 * 5. have `contains` method returns whether specified SuspendToken is in schedule queue;
 * 6. have `suspend` method that suspends specified coroutine;
 * 7. have `cancel` method that cancels specified SuspendToken.
 */
template<class scheduler_t>
concept CoroutineSchedulerConcept = std::constructible_from<scheduler_t, memory_resource::unsynchronized_pool_resource &>
                                    && requires(scheduler_t && s, ScheduleEvent::EventT schedule_event, SuspendToken token) {
  { scheduler_t::get() } noexcept -> std::same_as<scheduler_t &>;
  { s.done() } noexcept -> std::convertible_to<bool>;
  { s.schedule(schedule_event) } noexcept -> std::same_as<ScheduleStatus>;
  { s.contains(token) } noexcept -> std::convertible_to<bool>;
  { s.suspend(token) } noexcept -> std::same_as<void>;
  { s.cancel(token) } noexcept -> std::same_as<void>;
};

// === SimpleCoroutineScheduler ===================================================================

class SimpleCoroutineScheduler {
  template<hashable Key, typename Value>
  using unordered_map = memory_resource::stl::unordered_map<Key, Value, memory_resource::unsynchronized_pool_resource>;

  template<hashable T>
  using unordered_set = memory_resource::stl::unordered_set<T, memory_resource::unsynchronized_pool_resource>;

  template<typename T>
  using deque = memory_resource::stl::deque<T, memory_resource::unsynchronized_pool_resource>;

  deque<std::coroutine_handle<>> yield_coros;
  deque<std::coroutine_handle<>> awaiting_for_stream_coros;
  unordered_map<uint64_t, std::coroutine_handle<>> awaiting_for_update_coros;

  unordered_set<SuspendToken> suspend_tokens;

  ScheduleStatus scheduleOnNoEvent() noexcept;
  ScheduleStatus scheduleOnIncomingStream() noexcept;
  ScheduleStatus scheduleOnStreamUpdate(uint64_t) noexcept;
  ScheduleStatus scheduleOnYield() noexcept;

public:
  explicit SimpleCoroutineScheduler(memory_resource::unsynchronized_pool_resource &memory_resource) noexcept
    : yield_coros(deque<std::coroutine_handle<>>::allocator_type{memory_resource})
    , awaiting_for_stream_coros(deque<std::coroutine_handle<>>::allocator_type{memory_resource})
    , awaiting_for_update_coros(unordered_map<uint64_t, std::coroutine_handle<>>::allocator_type{memory_resource})
    , suspend_tokens(unordered_set<SuspendToken>::allocator_type{memory_resource}) {}

  static SimpleCoroutineScheduler &get() noexcept;

  bool done() const noexcept {
    return suspend_tokens.empty();
  }

  ScheduleStatus schedule(ScheduleEvent::EventT) noexcept;

  bool contains(SuspendToken token) const noexcept {
    return suspend_tokens.contains(token);
  }
  void suspend(SuspendToken) noexcept;
  void cancel(SuspendToken) noexcept;
};
