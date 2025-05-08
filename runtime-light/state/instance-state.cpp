// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/state/instance-state.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>
#include <utility>

#include "common/tl/constants/common.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/core/globals/php-init-scripts.h"
#include "runtime-light/core/globals/php-script-globals.h"
#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/scheduler/scheduler.h"
#include "runtime-light/server/http/init-functions.h"
#include "runtime-light/server/job-worker/init-functions.h"
#include "runtime-light/server/rpc/init-functions.h"
#include "runtime-light/state/component-state.h"
#include "runtime-light/stdlib/fork/fork-functions.h"
#include "runtime-light/stdlib/fork/fork-state.h"
#include "runtime-light/stdlib/time/time-functions.h"
#include "runtime-light/streams/streams.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/utils/logs.h"

namespace {

template<image_kind kind>
consteval std::string_view resolve_sapi_name() noexcept {
  if constexpr (kind == image_kind::cli) {
    return "cli";
  } else if constexpr (kind == image_kind::server) {
    return "server";
  } else if constexpr (kind == image_kind::oneshot) {
    return "oneshot";
  } else if constexpr (kind == image_kind::multishot) {
    return "multishot";
  } else {
    return "invalid interface";
  }
}

int32_t merge_output_buffers() noexcept {
  auto& instance_st{InstanceState::get()};
  Response& response{instance_st.response};
  kphp::log::assertion(response.current_buffer >= 0);

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

// === initialization =============================================================================

void InstanceState::init_script_execution() noexcept {
  runtime_context.init();
  kphp::coro::task<> script_task;
  init_php_scripts_in_each_worker(php_script_mutable_globals_singleton, script_task);

  auto main_task{std::invoke(
      [](kphp::coro::task<> script_task) noexcept -> kphp::coro::task<> {
        // wrap script with additional check for unhandled exception
        script_task = std::invoke(
            [](kphp::coro::task<> script_task) noexcept -> kphp::coro::task<> {
              co_await script_task;
              if (auto exception{std::move(ForkInstanceState::get().current_info().get().thrown_exception)}; !exception.is_null()) [[unlikely]] {
                kphp::log::error("unhandled exception '{}' at {}:{}", exception.get_class(), exception.get()->$file.c_str(), exception.get()->$line);
              }
            },
            std::move(script_task));
        kphp::log::assertion(co_await f$wait_concurrently(co_await start_fork_t{std::move(script_task)}));
      },
      std::move(script_task))};
  scheduler.suspend({main_task.get_handle(), WaitEvent::Rechedule{}});
  main_task_ = std::move(main_task);
}

kphp::coro::task<> InstanceState::init_cli_instance() noexcept {
  instance_kind_ = instance_kind::cli;

  auto& superglobals{php_script_mutable_globals_singleton.get_superglobals()};
  using namespace PhpServerSuperGlobalIndices;
  superglobals.v$argc = static_cast<int64_t>(0);
  superglobals.v$argv = array<mixed>{};
  superglobals.v$_SERVER.set_value(string{ARGC.data(), ARGC.size()}, superglobals.v$argc);
  superglobals.v$_SERVER.set_value(string{ARGV.data(), ARGV.size()}, superglobals.v$argv);
  superglobals.v$_SERVER.set_value(string{PHP_SELF.data(), PHP_SELF.size()}, string{});
  superglobals.v$_SERVER.set_value(string{SCRIPT_NAME.data(), SCRIPT_NAME.size()}, string{});
  standard_stream_ = co_await wait_for_incoming_stream_t{};
}

kphp::coro::task<> InstanceState::init_server_instance() noexcept {
  auto init_k2_invoke_http{[](tl::TLBuffer& tlb) noexcept {
    tl::K2InvokeHttp invoke_http{};
    if (!invoke_http.fetch(tlb)) [[unlikely]] {
      kphp::log::error("erroneous http request");
    }
    kphp::http::init_server(std::move(invoke_http));
  }};
  auto init_k2_invoke_rpc{[](tl::TLBuffer& tlb) noexcept {
    tl::K2InvokeRpc invoke_rpc{};
    if (!invoke_rpc.fetch(tlb)) [[unlikely]] {
      kphp::log::error("erroneous rpc request");
    }
    kphp::rpc::init_server(std::move(invoke_rpc));
  }};
  auto init_k2_invoke_jw{[](tl::TLBuffer& tlb) noexcept {
    tl::K2InvokeJobWorker invoke_jw{};
    if (!invoke_jw.fetch(tlb)) [[unlikely]] {
      kphp::log::error("erroneous job worker request");
    }
    init_job_server(invoke_jw);
  }};

  static constexpr std::string_view SERVER_SOFTWARE_VALUE = "K2/KPHP";
  static constexpr std::string_view SERVER_SIGNATURE_VALUE = "K2/KPHP Server v0.0.1";

  { // common initialization
    auto& server{php_script_mutable_globals_singleton.get_superglobals().v$_SERVER};
    using namespace PhpServerSuperGlobalIndices;
    server.set_value(string{SERVER_SOFTWARE.data(), SERVER_SOFTWARE.size()}, string{SERVER_SOFTWARE_VALUE.data(), SERVER_SOFTWARE_VALUE.size()});
    server.set_value(string{SERVER_SIGNATURE.data(), SERVER_SIGNATURE.size()}, string{SERVER_SIGNATURE_VALUE.data(), SERVER_SIGNATURE_VALUE.size()});
  }

  const auto stream_d{co_await wait_for_incoming_stream_t{}};
  const auto [buffer, size]{co_await read_all_from_stream(stream_d)};

  tl::TLBuffer tlb;
  tlb.store_bytes({buffer.get(), static_cast<size_t>(size)});

  switch (const auto magic{tlb.lookup_trivial<uint32_t>().value_or(TL_ZERO)}) { // lookup magic
  case tl::K2_INVOKE_HTTP_MAGIC: {
    instance_kind_ = instance_kind::http_server;
    standard_stream_ = stream_d;
    init_k2_invoke_http(tlb);
    break;
  }
  case tl::K2_INVOKE_RPC_MAGIC: {
    instance_kind_ = instance_kind::rpc_server;
    standard_stream_ = stream_d;
    init_k2_invoke_rpc(tlb);
    break;
  }
  case tl::K2_INVOKE_JOB_WORKER_MAGIC: {
    instance_kind_ = instance_kind::job_server;
    standard_stream_ = stream_d;
    init_k2_invoke_jw(tlb);
    // release standard stream in case of a no reply job worker since we don't need that stream anymore
    if (JobWorkerServerInstanceState::get().kind == JobWorkerServerInstanceState::Kind::NoReply) {
      release_stream(standard_stream_);
      standard_stream_ = k2::INVALID_PLATFORM_DESCRIPTOR;
    }
    break;
  }
  default:
    kphp::log::error("unexpected server request with magic: {:#x}", magic);
  }
}

template<image_kind kind>
kphp::coro::task<> InstanceState::run_instance_prologue() noexcept {
  static_assert(kind != image_kind::invalid);
  image_kind_ = kind;

  // common initialization
  {
    const auto time_mcs{f$_microtime_float()};
    constexpr auto sapi_name{resolve_sapi_name<kind>()};

    auto& superglobals{php_script_mutable_globals_singleton.get_superglobals()};
    superglobals.v$_ENV = ComponentState::get().env;

    using namespace PhpServerSuperGlobalIndices;
    superglobals.v$_SERVER.set_value(string{REQUEST_TIME.data(), REQUEST_TIME.size()}, static_cast<int64_t>(time_mcs));
    superglobals.v$_SERVER.set_value(string{REQUEST_TIME_FLOAT.data(), REQUEST_TIME_FLOAT.size()}, static_cast<double>(time_mcs));
    superglobals.v$d$PHP_SAPI = string{sapi_name.data(), sapi_name.size()};
  }

  // specific initialization
  if constexpr (kind == image_kind::cli) {
    co_await init_cli_instance();
  } else if constexpr (kind == image_kind::server) {
    co_await init_server_instance();
  }
}

template kphp::coro::task<> InstanceState::run_instance_prologue<image_kind::cli>();
template kphp::coro::task<> InstanceState::run_instance_prologue<image_kind::server>();
template kphp::coro::task<> InstanceState::run_instance_prologue<image_kind::oneshot>();
template kphp::coro::task<> InstanceState::run_instance_prologue<image_kind::multishot>();

// === finalization ===============================================================================

kphp::coro::task<> InstanceState::finalize_cli_instance() noexcept {
  const auto& output{response.output_buffers[merge_output_buffers()]};
  if (co_await write_all_to_stream(standard_stream(), output.buffer(), output.size()) != output.size()) [[unlikely]] {
    kphp::log::error("can't write output to stream {}", standard_stream());
  }
}

kphp::coro::task<> InstanceState::finalize_server_instance() noexcept {
  switch (instance_kind()) {
  case instance_kind::http_server: {
    co_await kphp::http::finalize_server(response.output_buffers[merge_output_buffers()]);
    break;
  }
  case instance_kind::rpc_server:
  case instance_kind::job_server:
    break;
  default:
    kphp::log::error("unexpected instance kind: {}", std::to_underlying(instance_kind()));
  }
  co_return;
}

kphp::coro::task<> InstanceState::run_instance_epilogue() noexcept {
  if (std::exchange(shutdown_state_, shutdown_state::in_progress) == shutdown_state::not_started) [[likely]] {
    for (auto& sf : shutdown_functions) {
      co_await sf;
    }
  }

  // to prevent performing the finalization twice
  if (shutdown_state_ == shutdown_state::finished) [[unlikely]] {
    co_return;
  }

  switch (image_kind()) {
  case image_kind::oneshot:
  case image_kind::multishot:
    break;
  case image_kind::cli: {
    co_await finalize_cli_instance();
    break;
  }
  case image_kind::server: {
    co_await finalize_server_instance();
    break;
  }
  default: {
    kphp::log::error("unexpected image kind: {}", std::to_underlying(image_kind()));
  }
  }
  release_all_streams();
  shutdown_state_ = shutdown_state::finished;
}

// ================================================================================================

void InstanceState::process_platform_updates() noexcept {
  for (;;) {
    // check if platform asked for yield
    if (static_cast<bool>(k2::control_flags()->please_yield.load())) { // tell the scheduler that we are about to yield
      kphp::log::debug("platform set 'please_yield=true'");
      const auto schedule_status{scheduler.schedule(ScheduleEvent::Yield{})};
      poll_status = schedule_status == ScheduleStatus::Error ? k2::PollStatus::PollFinishedError : k2::PollStatus::PollReschedule;
      return;
    }

    // try taking update from the platform
    if (uint64_t stream_d{}; static_cast<bool>(k2::take_update(std::addressof(stream_d)))) {
      if (opened_streams_.contains(stream_d)) { // update on opened stream
        kphp::log::debug("got update on stream {}", stream_d);
        switch (scheduler.schedule(ScheduleEvent::UpdateOnStream{.stream_d = stream_d})) {
        case ScheduleStatus::Resumed: { // scheduler's resumed a coroutine waiting for update
          break;
        }
        case ScheduleStatus::Skipped: { // no one is waiting for the event yet, so just save it
          pending_updates_.insert(stream_d);
          break;
        }
        case ScheduleStatus::Error: { // something bad's happened, stop execution
          poll_status = k2::PollStatus::PollFinishedError;
          return;
        }
        }
      } else { // update on incoming stream
        kphp::log::debug("got incoming stream {}", stream_d);
        incoming_streams_.push_back(stream_d);
        opened_streams_.insert(stream_d);
        if (const auto schedule_status{scheduler.schedule(ScheduleEvent::IncomingStream{.stream_d = stream_d})}; schedule_status == ScheduleStatus::Error) {
          poll_status = k2::PollStatus::PollFinishedError;
          return;
        }
      }
    } else { // we'are out of updates so let the scheduler do whatever it wants
      kphp::log::debug("got no events from platform");
      switch (scheduler.schedule(ScheduleEvent::NoEvent{})) {
      case ScheduleStatus::Resumed: { // scheduler's resumed some coroutine, so let's continue scheduling
        break;
      }
      case ScheduleStatus::Skipped: { // scheduler's done nothing, so it's either scheduled all coroutines or is waiting for events
        poll_status = scheduler.done() ? k2::PollStatus::PollFinishedOk : k2::PollStatus::PollBlocked;
        return;
      }
      case ScheduleStatus::Error: { // something bad's happened, stop execution
        poll_status = k2::PollStatus::PollFinishedError;
        return;
      }
      }
    }
  }
  std::unreachable();
}

uint64_t InstanceState::take_incoming_stream() noexcept {
  if (incoming_streams_.empty()) [[unlikely]] {
    kphp::log::warning("can't take incoming stream since there is not one");
    return k2::INVALID_PLATFORM_DESCRIPTOR;
  }
  const auto stream_d{incoming_streams_.front()};
  incoming_streams_.pop_front();
  kphp::log::debug("take incoming stream {}", stream_d);
  return stream_d;
}

std::pair<uint64_t, int32_t> InstanceState::open_stream(std::string_view component_name, k2::stream_kind stream_kind) noexcept {
  uint64_t stream_d{};
  int32_t error_code{};
  switch (stream_kind) {
  case k2::stream_kind::component:
    error_code = k2::open(std::addressof(stream_d), component_name.size(), component_name.data());
    break;
  case k2::stream_kind::tcp:
    error_code = k2::tcp_connect(std::addressof(stream_d), component_name.data(), component_name.size());
    break;
  case k2::stream_kind::udp:
    error_code = k2::udp_connect(std::addressof(stream_d), component_name.data(), component_name.size());
    break;
  default:
    error_code = k2::errno_einval;
    break;
  }

  if (error_code == k2::errno_ok) [[likely]] {
    opened_streams_.insert(stream_d);
    kphp::log::debug("opened a stream {} to {}", stream_d, component_name);
  } else {
    kphp::log::warning("can't open a stream to {}", component_name);
  }
  return {stream_d, error_code};
}

std::pair<uint64_t, int32_t> InstanceState::set_timer(std::chrono::nanoseconds duration) noexcept {
  uint64_t timer_d{};
  int32_t error_code{k2::new_timer(std::addressof(timer_d), static_cast<uint64_t>(duration.count()))};

  if (error_code == k2::errno_ok) [[likely]] {
    opened_streams_.insert(timer_d);
    kphp::log::debug("set timer {} for {} seconds", timer_d, std::chrono::duration<double>(duration).count());
  } else {
    kphp::log::warning("can't set a timer for {} seconds", std::chrono::duration<double>(duration).count());
  }
  return {timer_d, error_code};
}

void InstanceState::release_stream(uint64_t stream_d) noexcept {
  if (stream_d == standard_stream_) {
    standard_stream_ = k2::INVALID_PLATFORM_DESCRIPTOR;
  }
  opened_streams_.erase(stream_d);
  pending_updates_.erase(stream_d); // also erase pending updates if exists
  k2::free_descriptor(stream_d);
  kphp::log::debug("released a stream {}", stream_d);
}

void InstanceState::release_all_streams() noexcept {
  standard_stream_ = k2::INVALID_PLATFORM_DESCRIPTOR;
  for (const auto stream_d : opened_streams_) {
    k2::free_descriptor(stream_d);
    kphp::log::debug("released a stream {}", stream_d);
  }
  opened_streams_.clear();
  pending_updates_.clear();
  incoming_streams_.clear();
}
