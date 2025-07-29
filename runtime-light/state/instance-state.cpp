// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/state/instance-state.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>
#include <utility>

#include "common/tl/constants/common.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/core/globals/php-init-scripts.h"
#include "runtime-light/core/globals/php-script-globals.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/server/cli/init-functions.h"
#include "runtime-light/server/http/init-functions.h"
#include "runtime-light/server/rpc/init-functions.h"
#include "runtime-light/state/component-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/fork/fork-functions.h"
#include "runtime-light/stdlib/fork/fork-state.h"
#include "runtime-light/stdlib/time/time-functions.h"
#include "runtime-light/streams/stream.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"

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
                kphp::log::error("unhandled exception {}", std::move(exception));
              }
            },
            std::move(script_task));
        kphp::log::assertion(co_await f$wait_concurrently(kphp::forks::start(std::move(script_task))));
      },
      std::move(script_task))};
  // initialize async stack
  auto& main_task_async_stack_frame{main_task.get_handle().promise().get_async_stack_frame()};
  main_task_async_stack_frame.async_stack_root = std::addressof(coroutine_instance_state.coroutine_stack_root);
  coroutine_instance_state.coroutine_stack_root.top_async_stack_frame = std::addressof(main_task_async_stack_frame);
  // spawn main task onto the scheduler
  kphp::log::assertion(io_scheduler.spawn(std::move(main_task)));
}

kphp::coro::task<> InstanceState::init_cli_instance() noexcept {
  static constexpr size_t OUTPUT_STREAM_CAPACITY = 0;

  instance_kind_ = instance_kind::cli;
  auto opt_output_stream{co_await kphp::component::stream::accept(OUTPUT_STREAM_CAPACITY)};
  kphp::log::assertion(opt_output_stream.has_value());
  kphp::cli::init_cli_server(std::move(*opt_output_stream));
}

kphp::coro::task<> InstanceState::init_server_instance() noexcept {
  static constexpr std::string_view SERVER_SOFTWARE_VALUE = "K2/KPHP";
  static constexpr std::string_view SERVER_SIGNATURE_VALUE = "K2/KPHP Server v0.0.1";

  { // common initialization
    auto& server{php_script_mutable_globals_singleton.get_superglobals().v$_SERVER};
    using namespace PhpServerSuperGlobalIndices;
    server.set_value(string{SERVER_SOFTWARE.data(), SERVER_SOFTWARE.size()}, string{SERVER_SOFTWARE_VALUE.data(), SERVER_SOFTWARE_VALUE.size()});
    server.set_value(string{SERVER_SIGNATURE.data(), SERVER_SIGNATURE.size()}, string{SERVER_SIGNATURE_VALUE.data(), SERVER_SIGNATURE_VALUE.size()});
  }

  auto opt_request_stream{co_await kphp::component::stream::accept()};
  kphp::log::assertion(opt_request_stream.has_value());
  auto request_stream{std::move(*opt_request_stream)};
  if (auto expected{co_await request_stream.read()}; !expected) [[unlikely]] {
    kphp::log::error("failed to read a request: stream -> {}", request_stream.descriptor());
  }

  tl::TLBuffer tlb;
  tlb.store_bytes({reinterpret_cast<const char*>(request_stream.data().data()), request_stream.data().size()});

  switch (const auto magic{tlb.lookup_trivial<uint32_t>().value_or(TL_ZERO)}) { // lookup magic
  case tl::K2_INVOKE_HTTP_MAGIC:
    instance_kind_ = instance_kind::http_server;
    kphp::http::init_server(std::move(request_stream), tlb);
    break;
  case tl::K2_INVOKE_RPC_MAGIC:
    instance_kind_ = instance_kind::rpc_server;
    kphp::rpc::init_server(std::move(request_stream), tlb);
    break;
  case tl::K2_INVOKE_JOB_WORKER_MAGIC:
    kphp::log::error("job worker server is currently not supported");
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
  co_await kphp::cli::finalize_cli_server();
}

kphp::coro::task<> InstanceState::finalize_server_instance() const noexcept {
  switch (instance_kind()) {
  case instance_kind::http_server: {
    co_await kphp::http::finalize_server();
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
  case image_kind::cli:
    co_await finalize_cli_instance();
    break;
  case image_kind::server:
    co_await finalize_server_instance();
    break;
  default:
    kphp::log::error("unexpected image kind: {}", std::to_underlying(image_kind()));
  }
  shutdown_state_ = shutdown_state::finished;
}
