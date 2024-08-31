// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/component/component.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <utility>

#include "runtime-core/utils/kphp-assert-core.h"
#include "runtime-light/core/globals/php-init-scripts.h"
#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/header.h"
#include "runtime-light/scheduler/scheduler.h"
#include "runtime-light/streams/streams.h"
#include "runtime-light/utils/context.h"
#include "runtime-light/utils/json-functions.h"

namespace {

constexpr uint32_t K2_INVOKE_HTTP_MAGIC = 0xd909efe8;
constexpr uint32_t K2_INVOKE_JOB_WORKER_MAGIC = 0x437d7312;

void init_http_superglobals(const string &http_query) noexcept {
  auto &component_ctx{*get_component_context()};
  component_ctx.php_script_mutable_globals_singleton.get_superglobals().v$_SERVER.set_value(string{"QUERY_TYPE"}, string{"http"});
  component_ctx.php_script_mutable_globals_singleton.get_superglobals().v$_POST = f$json_decode(http_query, true);
}

task_t<uint64_t> init_kphp_cli_component() noexcept {
  co_return co_await wait_for_incoming_stream_t{};
}

task_t<uint64_t> init_kphp_server_component() noexcept {
  uint32_t magic{};
  const auto stream_d{co_await wait_for_incoming_stream_t{}};
  const auto read{co_await read_exact_from_stream(stream_d, reinterpret_cast<char *>(std::addressof(magic)), sizeof(uint32_t))};
  php_assert(read == sizeof(uint32_t));
  if (magic == K2_INVOKE_HTTP_MAGIC) {
    const auto [buffer, size]{co_await read_all_from_stream(stream_d)};
    init_http_superglobals(string{buffer, static_cast<string::size_type>(size)});
    get_platform_context()->allocator.free(buffer);
  } else if (magic == K2_INVOKE_JOB_WORKER_MAGIC) {
    php_error("not implemented");
  } else {
    php_error("server got unexpected type of request: 0x%x", magic);
  }

  co_return stream_d;
}

int32_t merge_output_buffers(ComponentState &component_ctx) noexcept {
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
  if (standard_stream() == INVALID_PLATFORM_DESCRIPTOR) {
    poll_status = PollStatus::PollFinishedError;
    co_return;
  }

  const auto &buffer{response.output_buffers[merge_output_buffers(*this)]};
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
