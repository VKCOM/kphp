// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/scheduler/scheduler.h"

#include <algorithm>
#include <coroutine>
#include <cstdint>
#include <type_traits>
#include <variant>

#include "runtime-light/component/component.h"
#include "runtime-light/utils/context.h"

// === SimpleCoroutineScheduler ===================================================================

SimpleCoroutineScheduler::SimpleCoroutineScheduler(memory_resource::unsynchronized_pool_resource &memory_resource) noexcept
  : yield_coros(deque<std::coroutine_handle<>>::allocator_type{memory_resource})
  , awaiting_for_stream_coros(deque<std::coroutine_handle<>>::allocator_type{memory_resource})
  , awaiting_for_update_coros(unordered_map<uint64_t, std::coroutine_handle<>>::allocator_type{memory_resource}) {}

SimpleCoroutineScheduler &SimpleCoroutineScheduler::get() noexcept {
  return get_component_context()->scheduler;
}

bool SimpleCoroutineScheduler::done() const noexcept {
  return yield_coros.empty() && awaiting_for_stream_coros.empty() && awaiting_for_update_coros.empty();
}

ScheduleStatus SimpleCoroutineScheduler::scheduleOnNoEvent() noexcept {
  if (yield_coros.empty()) {
    return ScheduleStatus::Skipped;
  }
  const auto coro{yield_coros.front()};
  yield_coros.pop_front();
  coro.resume();
  return ScheduleStatus::Resumed;
}

ScheduleStatus SimpleCoroutineScheduler::scheduleOnIncomingStream() noexcept {
  if (awaiting_for_stream_coros.empty()) {
    return ScheduleStatus::Skipped;
  }
  const auto coro{awaiting_for_stream_coros.front()};
  awaiting_for_stream_coros.pop_front();
  coro.resume();
  return ScheduleStatus::Resumed;
}

ScheduleStatus SimpleCoroutineScheduler::scheduleOnStreamUpdate(uint64_t stream_d) noexcept {
  if (stream_d == INVALID_PLATFORM_DESCRIPTOR) {
    return ScheduleStatus::Error;
  } else if (const auto it_coro{awaiting_for_update_coros.find(stream_d)}; it_coro != awaiting_for_update_coros.cend()) {
    const auto coro{it_coro->second};
    awaiting_for_update_coros.erase(it_coro);
    coro.resume();
    return ScheduleStatus::Resumed;
  } else {
    return ScheduleStatus::Skipped;
  }
}

ScheduleStatus SimpleCoroutineScheduler::scheduleOnYield() noexcept {
  return ScheduleStatus::Skipped;
}

ScheduleStatus SimpleCoroutineScheduler::schedule(ScheduleEvent::EventT event) noexcept {
  return std::visit(
    [this](auto &&event) {
      using event_t = std::remove_cvref_t<decltype(event)>;
      if constexpr (std::is_same_v<event_t, ScheduleEvent::NoEvent>) {
        return scheduleOnNoEvent();
      } else if constexpr (std::is_same_v<event_t, ScheduleEvent::IncomingStream>) {
        return scheduleOnIncomingStream();
      } else if constexpr (std::is_same_v<event_t, ScheduleEvent::UpdateOnStream>) {
        return scheduleOnStreamUpdate(event.stream_d);
      } else if constexpr (std::is_same_v<event_t, ScheduleEvent::UpdateOnTimer>) {
        return scheduleOnStreamUpdate(event.timer_d);
      } else if constexpr (std::is_same_v<event_t, ScheduleEvent::Yield>) {
        return scheduleOnYield();
      } else {
        static_assert(false, "non-exhaustive visitor");
      }
    },
    event);
}

void SimpleCoroutineScheduler::suspend(SuspendToken token) noexcept {
  const auto [coro, event]{token};
  std::visit(
    [this, coro](auto &&event) {
      using event_t = std::remove_cvref_t<decltype(event)>;
      if constexpr (std::is_same_v<event_t, WaitEvent::Rechedule>) {
        yield_coros.push_back(coro);
      } else if constexpr (std::is_same_v<event_t, WaitEvent::IncomingStream>) {
        awaiting_for_stream_coros.push_back(coro);
      } else if constexpr (std::is_same_v<event_t, WaitEvent::UpdateOnStream>) {
        if (event.stream_d == INVALID_PLATFORM_DESCRIPTOR) {
          return;
        }
        awaiting_for_update_coros.emplace(event.stream_d, coro);
      } else if constexpr (std::is_same_v<event_t, WaitEvent::UpdateOnTimer>) {
        if (event.timer_d == INVALID_PLATFORM_DESCRIPTOR) {
          return;
        }
        awaiting_for_update_coros.emplace(event.timer_d, coro);
      } else {
        static_assert(false, "non-exhaustive visitor");
      }
    },
    event);
}

void SimpleCoroutineScheduler::cancel(SuspendToken token) noexcept {
  const auto [coro, event]{token};
  std::visit(
    [this, coro](auto &&event) {
      using event_t = std::remove_cvref_t<decltype(event)>;
      if constexpr (std::is_same_v<event_t, WaitEvent::Rechedule>) {
        yield_coros.erase(std::find(yield_coros.cbegin(), yield_coros.cend(), coro));
      } else if constexpr (std::is_same_v<event_t, WaitEvent::IncomingStream>) {
        awaiting_for_stream_coros.erase(std::find(awaiting_for_stream_coros.cbegin(), awaiting_for_stream_coros.cend(), coro));
      } else if constexpr (std::is_same_v<event_t, WaitEvent::UpdateOnStream>) {
        awaiting_for_update_coros.erase(event.stream_d);
      } else if constexpr (std::is_same_v<event_t, WaitEvent::UpdateOnTimer>) {
        awaiting_for_update_coros.erase(event.timer_d);
      } else {
        static_assert(false, "non-exhaustive visitor");
      }
    },
    event);
}
