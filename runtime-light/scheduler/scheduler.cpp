// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/scheduler/scheduler.h"

#include <algorithm>
#include <coroutine>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <variant>

#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/state/instance-state.h"

// === SimpleCoroutineScheduler ===================================================================

SimpleCoroutineScheduler& SimpleCoroutineScheduler::get() noexcept {
  return InstanceState::get().scheduler;
}

ScheduleStatus SimpleCoroutineScheduler::scheduleOnNoEvent() noexcept {
  if (yield_tokens.empty()) {
    return ScheduleStatus::Skipped;
  }
  const auto token{yield_tokens.front()};
  yield_tokens.pop_front();
  suspend_tokens.erase(token);
  kphp::coro::resume(token.first, coroutine_instance_state.coroutine_stack_root);
  return ScheduleStatus::Resumed;
}

ScheduleStatus SimpleCoroutineScheduler::scheduleOnIncomingStream() noexcept {
  if (awaiting_for_stream_tokens.empty()) {
    return ScheduleStatus::Skipped;
  }
  const auto token{awaiting_for_stream_tokens.front()};
  awaiting_for_stream_tokens.pop_front();
  suspend_tokens.erase(token);
  kphp::coro::resume(token.first, coroutine_instance_state.coroutine_stack_root);
  return ScheduleStatus::Resumed;
}

ScheduleStatus SimpleCoroutineScheduler::scheduleOnStreamUpdate(uint64_t stream_d) noexcept {
  if (stream_d == k2::INVALID_PLATFORM_DESCRIPTOR) {
    return ScheduleStatus::Error;
  } else if (const auto it_token{awaiting_for_update_tokens.find(stream_d)}; it_token != awaiting_for_update_tokens.cend()) {
    const auto token{it_token->second};
    awaiting_for_update_tokens.erase(it_token);
    suspend_tokens.erase(token);
    kphp::coro::resume(token.first, coroutine_instance_state.coroutine_stack_root);
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
      [this](auto&& event) noexcept {
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
  suspend_tokens.emplace(token);
  std::visit(
      [this, token](auto&& event) noexcept {
        using event_t = std::remove_cvref_t<decltype(event)>;
        if constexpr (std::is_same_v<event_t, WaitEvent::Reschedule>) {
          yield_tokens.push_back(token);
        } else if constexpr (std::is_same_v<event_t, WaitEvent::IncomingStream>) {
          awaiting_for_stream_tokens.push_back(token);
        } else if constexpr (std::is_same_v<event_t, WaitEvent::UpdateOnStream>) {
          if (event.stream_d == k2::INVALID_PLATFORM_DESCRIPTOR) {
            return;
          }
          awaiting_for_update_tokens.emplace(event.stream_d, token);
        } else if constexpr (std::is_same_v<event_t, WaitEvent::UpdateOnTimer>) {
          if (event.timer_d == k2::INVALID_PLATFORM_DESCRIPTOR) {
            return;
          }
          awaiting_for_update_tokens.emplace(event.timer_d, token);
        } else {
          static_assert(false, "non-exhaustive visitor");
        }
      },
      token.second);
}

void SimpleCoroutineScheduler::cancel(SuspendToken token) noexcept {
  suspend_tokens.erase(token);
  std::visit(
      [this, token](auto&& event) noexcept {
        using event_t = std::remove_cvref_t<decltype(event)>;
        if constexpr (std::is_same_v<event_t, WaitEvent::Reschedule>) {
          const auto it_token{std::find(yield_tokens.cbegin(), yield_tokens.cend(), token)};
          if (it_token != yield_tokens.cend()) {
            yield_tokens.erase(it_token);
          }
        } else if constexpr (std::is_same_v<event_t, WaitEvent::IncomingStream>) {
          const auto it_token{std::find(awaiting_for_stream_tokens.cbegin(), awaiting_for_stream_tokens.cend(), token)};
          if (it_token != awaiting_for_stream_tokens.cend()) {
            awaiting_for_stream_tokens.erase(it_token);
          }
        } else if constexpr (std::is_same_v<event_t, WaitEvent::UpdateOnStream>) {
          awaiting_for_update_tokens.erase(event.stream_d);
        } else if constexpr (std::is_same_v<event_t, WaitEvent::UpdateOnTimer>) {
          awaiting_for_update_tokens.erase(event.timer_d);
        } else {
          static_assert(false, "non-exhaustive visitor");
        }
      },
      token.second);
}
