// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/component/component.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/component/init-functions.h"
#include "runtime-light/core/globals/php-init-scripts.h"
#include "runtime-light/core/globals/php-script-globals.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/header.h"
#include "runtime-light/scheduler/scheduler.h"
#include "runtime-light/server/job-worker/job-worker-server-context.h"
#include "runtime-light/streams/streams.h"
#include "runtime-light/utils/context.h"

namespace {

int32_t merge_output_buffers() noexcept {
  auto &component_ctx{*get_component_context()};
  Response &response{component_ctx.response};
  php_assert(response.current_buffer >= 0);

  int32_t ob_first_not_empty{};
  while (ob_first_not_empty < response.current_buffer && response.output_buffers[ob_first_not_empty].size() == 0) {
    ++ob_first_not_empty; // TODO: optimize by precomputing final buffer's size to reserve enough space
  }
  for (auto i = ob_first_not_empty + 1; i <= response.current_buffer; i++) {
    response.output_buffers[ob_first_not_empty].append(response.output_buffers[i].c_str(), response.output_buffers[i].size());
  }
  return ob_first_not_empty;
}

} // namespace

void ComponentState::init_script_execution() noexcept {
  kphp_core_context.init();
  init_php_scripts_in_each_worker(php_script_mutable_globals_singleton, main_task_);
  scheduler.suspend(std::make_pair(main_task_.get_handle(), WaitEvent::Rechedule{}));
}

template<ComponentKind kind>
task_t<void> ComponentState::run_component_prologue() noexcept {
  static_assert(kind != ComponentKind::Invalid);
  component_kind_ = kind;

  // common initialization
  auto &superglobals{php_script_mutable_globals_singleton.get_superglobals()};
  superglobals.v$argc = static_cast<int64_t>(0); // TODO
  superglobals.v$argv = array<mixed>{};          // TODO
  {
    const auto &platform_ctx{*get_platform_context()};

    SystemTime sys_time{};
    platform_ctx.get_system_time(std::addressof(sys_time));
    const auto time_mcs{std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::nanoseconds{sys_time.since_epoch_ns}).count()};

    using namespace PhpServerSuperGlobalIndices;
    superglobals.v$_SERVER.set_value(string{ARGC, std::char_traits<char>::length(ARGC)}, superglobals.v$argc);
    superglobals.v$_SERVER.set_value(string{ARGV, std::char_traits<char>::length(ARGV)}, superglobals.v$argv);
    superglobals.v$_SERVER.set_value(string{PHP_SELF, std::char_traits<char>::length(PHP_SELF)}, string{}); // TODO: script name
    superglobals.v$_SERVER.set_value(string{SCRIPT_NAME, std::char_traits<char>::length(SCRIPT_NAME)}, string{});
    superglobals.v$_SERVER.set_value(string{REQUEST_TIME, std::char_traits<char>::length(REQUEST_TIME)}, static_cast<int64_t>(sys_time.since_epoch_ns));
    superglobals.v$_SERVER.set_value(string{REQUEST_TIME_FLOAT, std::char_traits<char>::length(REQUEST_TIME_FLOAT)}, static_cast<double>(time_mcs));
  }
  // TODO sapi, env

  // specific initialization
  if constexpr (kind == ComponentKind::CLI) {
    standard_stream_ = co_await init_kphp_cli_component();
  } else if constexpr (kind == ComponentKind::Server) {
    standard_stream_ = co_await init_kphp_server_component();
  }
}

template task_t<void> ComponentState::run_component_prologue<ComponentKind::CLI>();
template task_t<void> ComponentState::run_component_prologue<ComponentKind::Server>();
template task_t<void> ComponentState::run_component_prologue<ComponentKind::Oneshot>();
template task_t<void> ComponentState::run_component_prologue<ComponentKind::Multishot>();

task_t<void> ComponentState::run_component_epilogue() noexcept {
  if (component_kind_ == ComponentKind::Oneshot || component_kind_ == ComponentKind::Multishot) {
    co_return;
  }
  // do not flush output buffers if we are in job worker
  if (job_worker_server_component_context.kind != JobWorkerServerComponentContext::Kind::Invalid) {
    co_return;
  }
  if (standard_stream() == INVALID_PLATFORM_DESCRIPTOR) {
    poll_status = PollStatus::PollFinishedError;
    co_return;
  }

  const auto &buffer{response.output_buffers[merge_output_buffers()]};
  if ((co_await write_all_to_stream(standard_stream(), buffer.buffer(), buffer.size())) != buffer.size()) {
    php_warning("can't write component result to stream %" PRIu64, standard_stream());
  }
}

void ComponentState::process_platform_updates() noexcept {
  const auto &platform_ctx{*get_platform_context()};

  for (;;) {
    // check if platform asked for yield
    if (static_cast<bool>(platform_ctx.please_yield.load())) { // tell the scheduler that we are about to yield
      php_debug("platform asked for yield");
      const auto schedule_status{scheduler.schedule(ScheduleEvent::Yield{})};
      poll_status = schedule_status == ScheduleStatus::Error ? PollStatus::PollFinishedError : PollStatus::PollReschedule;
      return;
    }

    // try taking update from the platform
    if (uint64_t stream_d{}; static_cast<bool>(platform_ctx.take_update(std::addressof(stream_d)))) {
      if (opened_streams_.contains(stream_d)) { // update on opened stream
        php_debug("took update on stream %" PRIu64, stream_d);
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
        php_debug("got new incoming stream %" PRIu64, stream_d);
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

uint64_t ComponentState::open_stream(std::string_view component_name_view) noexcept {
  uint64_t stream_d{};
  if (const auto open_stream_res{get_platform_context()->open(component_name_view.size(), component_name_view.data(), std::addressof(stream_d))};
      open_stream_res != OpenStreamResult::OpenStreamOk) {
    php_warning("can't open stream to %s", component_name_view.data());
    return INVALID_PLATFORM_DESCRIPTOR;
  }
  opened_streams_.insert(stream_d);
  php_debug("opened a stream %" PRIu64 " to %s", stream_d, component_name_view.data());
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
