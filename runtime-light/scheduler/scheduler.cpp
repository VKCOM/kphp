// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/scheduler/scheduler.h"

#include <cinttypes>
#include <coroutine>
#include <cstdint>
#include <type_traits>
#include <variant>

#include "runtime-core/utils/kphp-assert-core.h"
#include "runtime-light/component/component.h"
#include "runtime-light/utils/context.h"

// === SimpleCoroutineScheduler ==============================================================================

SimpleCoroutineScheduler::SimpleCoroutineScheduler(memory_resource::unsynchronized_pool_resource &memory_resource) noexcept
  : yield_coros(deque<std::coroutine_handle<>>::allocator_type{memory_resource})
  , awaiting_for_stream_coros(deque<std::coroutine_handle<>>::allocator_type{memory_resource})
  , awaiting_for_update_coros(unordered_map<uint64_t, std::coroutine_handle<>>::allocator_type{memory_resource}) {}

SimpleCoroutineScheduler &SimpleCoroutineScheduler::get() noexcept {
  return get_component_context()->scheduler;
}

bool SimpleCoroutineScheduler::done() const noexcept {
  const auto finished_{yield_coros.empty() && awaiting_for_stream_coros.empty() && awaiting_for_update_coros.empty()};
  php_debug("SimpleCoroutineScheduler: done - %s", finished_ ? "true" : "false");
  return finished_;
}

ScheduleStatus SimpleCoroutineScheduler::scheduleOnNoEvent() noexcept {
  if (yield_coros.empty()) {
    php_debug("SimpleCoroutineScheduler: scheduleOnNoEvent skipped");
    return ScheduleStatus::Skipped;
  }
  php_debug("SimpleCoroutineScheduler: scheduleOnNoEvent resumed");
  const auto coro{yield_coros.front()};
  yield_coros.pop_front();
  coro.resume();
  return ScheduleStatus::Resumed;
}

ScheduleStatus SimpleCoroutineScheduler::scheduleOnIncomingStream() noexcept {
  if (awaiting_for_stream_coros.empty()) {
    php_debug("SimpleCoroutineScheduler: scheduleOnIncomingStream skipped");
    return ScheduleStatus::Skipped;
  }
  php_debug("SimpleCoroutineScheduler: scheduleOnIncomingStream resumed");
  const auto coro{awaiting_for_stream_coros.front()};
  awaiting_for_stream_coros.pop_front();
  coro.resume();
  return ScheduleStatus::Resumed;
}

ScheduleStatus SimpleCoroutineScheduler::scheduleOnStreamUpdate(uint64_t stream_d) noexcept {
  if (stream_d == INVALID_PLATFORM_DESCRIPTOR) {
    php_warning("SimpleCoroutineScheduler: scheduleOnStreamUpdate invalid stream %" PRIu64, stream_d);
    return ScheduleStatus::Error;
  } else if (const auto it_coro{awaiting_for_update_coros.find(stream_d)}; it_coro != awaiting_for_update_coros.cend()) {
    php_debug("SimpleCoroutineScheduler: scheduleOnStreamUpdate scheduled on %" PRIu64, stream_d);
    const auto coro{it_coro->second};
    awaiting_for_update_coros.erase(it_coro);
    coro.resume();
    return ScheduleStatus::Resumed;
  } else {
    php_debug("SimpleCoroutineScheduler: scheduleOnStreamUpdate skipped on %" PRIu64, stream_d);
    return ScheduleStatus::Skipped;
  }
}

ScheduleStatus SimpleCoroutineScheduler::scheduleOnYield() noexcept {
  php_debug("SimpleCoroutineScheduler: scheduleOnYield skipped");
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

void SimpleCoroutineScheduler::wait_for_update(std::coroutine_handle<> coro, uint64_t event_id) noexcept {
  php_debug("SimpleCoroutineScheduler: registered await on %" PRIu64, event_id);
  awaiting_for_update_coros.emplace(event_id, coro);
}

void SimpleCoroutineScheduler::wait_for_incoming_stream(std::coroutine_handle<> coro) noexcept {
  php_debug("SimpleCoroutineScheduler: register await for incoming stream");
  awaiting_for_stream_coros.push_back(coro);
}

void SimpleCoroutineScheduler::wait_for_reschedule(std::coroutine_handle<> coro) noexcept {
  php_debug("SimpleCoroutineScheduler: register wait for reschedule");
  yield_coros.push_back(coro);
}
