// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/component/component.h"

#include <chrono>
#include <cstdint>
#include <utility>

#include "runtime-core/utils/kphp-assert-core.h"
#include "runtime-light/core/globals/php-init-scripts.h"
#include "runtime-light/header.h"
#include "runtime-light/scheduler/scheduler.h"
#include "runtime-light/utils/context.h"

void ComponentState::init_script_execution() noexcept {
  kphp_core_context.init();
  init_php_scripts_in_each_worker(php_script_mutable_globals_singleton, main_task);
  scheduler.suspend(std::make_pair(main_task.get_handle(), WaitEvent::Rechedule{}));
}

void ComponentState::process_platform_updates() noexcept {
  const auto &platform_ctx{*get_platform_context()};

  for (;;) {
    // check if platform asked for yield
    if (static_cast<bool>(platform_ctx.please_yield.load())) { // tell the scheduler that we are about to yield
      php_debug("platform ask component to yield. Finish updates processing");
      const auto schedule_status{scheduler.schedule(ScheduleEvent::Yield{})};
      poll_status = schedule_status == ScheduleStatus::Error ? PollStatus::PollFinishedError : PollStatus::PollReschedule;
      return;
    }

    // try taking update from the platform
    if (uint64_t stream_d{}; static_cast<bool>(platform_ctx.take_update(std::addressof(stream_d)))) {
      if (opened_streams_.contains(stream_d)) { // update on opened stream
        php_debug("update on opened stream %lu\n", stream_d);
        switch (scheduler.schedule(ScheduleEvent::UpdateOnStream{.stream_d = stream_d})) {
          case ScheduleStatus::Resumed: { // scheduler's resumed a coroutine waiting for update
            break;
          }
          case ScheduleStatus::Skipped: { // no one is waiting for the event yet, so just save it
            pending_updates_.insert(stream_d);
            break;
          }
          case ScheduleStatus::Error: { // something bad's happened, stop execution
            poll_status = PollStatus::PollFinishedError;
            return;
          }
        }
      } else { // update on incoming stream
        php_debug("start processing new incoming stream %lu\n", stream_d);
        if (standard_stream_ != INVALID_PLATFORM_DESCRIPTOR) {
          php_warning("skip new incoming stream since previous one is not closed");
          release_stream(stream_d);
          continue;
        } // TODO: multiple incoming streams (except for http queries)
        standard_stream_ = stream_d;
        incoming_streams_.push_back(stream_d);
        opened_streams_.insert(stream_d);
        if (const auto schedule_status{scheduler.schedule(ScheduleEvent::IncomingStream{.stream_d = stream_d})}; schedule_status == ScheduleStatus::Error) {
          poll_status = PollStatus::PollFinishedError;
          return;
        }
      }
    } else { // we'are out of updates so let the scheduler do whatever it wants
      switch (scheduler.schedule(ScheduleEvent::NoEvent{})) {
        case ScheduleStatus::Resumed: { // scheduler's resumed some coroutine, so let's continue scheduling
          break;
        }
        case ScheduleStatus::Skipped: { // scheduler's done nothing, so it's either scheduled all coroutines or is waiting for events
          poll_status = scheduler.done() ? PollStatus::PollFinishedOk : PollStatus::PollBlocked;
          return;
        }
        case ScheduleStatus::Error: { // something bad's happened, stop execution
          poll_status = PollStatus::PollFinishedError;
          return;
        }
      }
    }
  }
  // unreachable code
  poll_status = PollStatus::PollFinishedError;
}

uint64_t ComponentState::take_incoming_stream() noexcept {
  if (incoming_streams_.empty()) {
    php_warning("can't take incoming stream cause we don't have them");
    return INVALID_PLATFORM_DESCRIPTOR;
  }
  const auto stream_d{incoming_streams_.front()};
  incoming_streams_.pop_front();
  php_debug("take incoming stream %" PRIu64, stream_d);
  return stream_d;
}

uint64_t ComponentState::open_stream(const string &component_name) noexcept {
  uint64_t stream_d{};
  if (const auto open_stream_res{get_platform_context()->open(component_name.size(), component_name.c_str(), std::addressof(stream_d))};
      open_stream_res != OpenStreamResult::OpenStreamOk) {
    php_warning("can't open stream to %s", component_name.c_str());
    return INVALID_PLATFORM_DESCRIPTOR;
  }
  opened_streams_.insert(stream_d);
  php_debug("opened a stream %" PRIu64 " to %s", stream_d, component_name.c_str());
  return stream_d;
}

uint64_t ComponentState::set_timer(std::chrono::nanoseconds duration) noexcept {
  uint64_t timer_d{};
  if (const auto set_timer_res{get_platform_context()->set_timer(std::addressof(timer_d), static_cast<uint64_t>(duration.count()))};
      set_timer_res != SetTimerResult::SetTimerOk) {
    php_warning("can't set timer for %.9f sec", std::chrono::duration<double>(duration).count());
    return INVALID_PLATFORM_DESCRIPTOR;
  }
  opened_streams_.insert(timer_d);
  php_debug("set timer %" PRIu64 " for %.9f sec", timer_d, std::chrono::duration<double>(duration).count());
  return timer_d;
}

void ComponentState::release_stream(uint64_t stream_d) noexcept {
  if (stream_d == standard_stream_) {
    standard_stream_ = INVALID_PLATFORM_DESCRIPTOR;
  }
  opened_streams_.erase(stream_d);
  pending_updates_.erase(stream_d); // also erase pending updates if exists
  get_platform_context()->free_descriptor(stream_d);
  php_debug("released a stream %" PRIu64, stream_d);
}

void ComponentState::release_all_streams() noexcept {
  const auto &platform_ctx{*get_platform_context()};
  standard_stream_ = INVALID_PLATFORM_DESCRIPTOR;
  for (const auto stream_d : opened_streams_) {
    platform_ctx.free_descriptor(stream_d);
    php_debug("released a stream %" PRIu64, stream_d);
  }
  opened_streams_.clear();
  pending_updates_.clear();
  incoming_streams_.clear();
}
