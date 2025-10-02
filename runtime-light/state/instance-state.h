// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/allocator/allocator-state.h"
#include "runtime-light/core/globals/php-script-globals.h"
#include "runtime-light/coroutine/coroutine-state.h"
#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/server/cli/cli-instance-state.h"
#include "runtime-light/server/http/http-server-state.h"
#include "runtime-light/server/job-worker/job-worker-server-state.h"
#include "runtime-light/server/rpc/rpc-server-state.h"
#include "runtime-light/state/component-state.h"
#include "runtime-light/stdlib/curl/curl-state.h"
#include "runtime-light/stdlib/diagnostics/contextual-logger.h"
#include "runtime-light/stdlib/diagnostics/error-handling-state.h"
#include "runtime-light/stdlib/fork/fork-state.h"
#include "runtime-light/stdlib/instance-cache/instance-cache-state.h"
#include "runtime-light/stdlib/job-worker/job-worker-client-state.h"
#include "runtime-light/stdlib/kml/kml-state.h"
#include "runtime-light/stdlib/math/math-state.h"
#include "runtime-light/stdlib/math/random-state.h"
#include "runtime-light/stdlib/output/output-state.h"
#include "runtime-light/stdlib/rpc/rpc-client-state.h"
#include "runtime-light/stdlib/serialization/serialization-state.h"
#include "runtime-light/stdlib/string/regex-state.h"
#include "runtime-light/stdlib/string/string-state.h"
#include "runtime-light/stdlib/system/system-state.h"
#include "runtime-light/stdlib/time/time-state.h"

/**
 * Supported kinds of KPHP images:
 * 1. cli — works the same way as regular PHP script does
 * 2. server — automatically accepts a stream and expects it to contain either http or job worker request
 * 3. oneshot — can only accept one incoming stream
 * 4. multishot — can accept any number of incoming streams
 */
enum class image_kind : uint8_t { invalid, cli, server, oneshot, multishot };

enum class instance_kind : uint8_t { invalid, cli, http_server, rpc_server, job_server, oneshot, multishot };

struct InstanceState final : vk::not_copyable {
  template<typename T>
  using unordered_set = kphp::stl::unordered_set<T, kphp::memory::script_allocator>;

  template<typename T>
  using deque = kphp::stl::deque<T, kphp::memory::script_allocator>;

  template<typename T>
  using list = kphp::stl::list<T, kphp::memory::script_allocator>;

  // It's important to use `{}` instead of `= default` here.
  // In the second case clang++ zeroes the whole structure.
  // It drastically ruins performance. Be careful!
  InstanceState() noexcept {
    kml_instance_state.init(ComponentState::get().kml_component_state.max_buffer_size());
  }

  static InstanceState& get() noexcept {
    return *k2::instance_state();
  }

  void init_script_execution() noexcept;

  template<image_kind>
  kphp::coro::task<> run_instance_prologue() noexcept;

  kphp::coro::task<> run_instance_epilogue() noexcept;

  image_kind image_kind() const noexcept {
    return image_kind_;
  }
  instance_kind instance_kind() const noexcept {
    return instance_kind_;
  }

  AllocatorState instance_allocator_state{INIT_INSTANCE_ALLOCATOR_SIZE, 0};
  kphp::log::contextual_logger instance_logger;

  kphp::coro::io_scheduler io_scheduler;
  CoroutineInstanceState coroutine_instance_state;
  ForkInstanceState fork_instance_state;
  PhpScriptMutableGlobals php_script_mutable_globals_singleton;

  RuntimeContext runtime_context;
  CLIInstanceInstance cli_instance_instate;
  OutputInstanceState output_instance_state;
  RpcClientInstanceState rpc_client_instance_state;
  RpcServerInstanceState rpc_server_instance_state;
  SerializationInstanceState serialization_instance_state;
  HttpServerInstanceState http_server_instance_state;
  JobWorkerClientInstanceState job_worker_client_instance_state;
  JobWorkerServerInstanceState job_worker_server_instance_state;
  InstanceCacheInstanceState instance_cache_instance_state;

  TimeInstanceState time_instance_state;
  MathInstanceState math_instance_state;
  RandomInstanceState random_instance_state;
  RegexInstanceState regex_instance_state;
  CurlInstanceState curl_instance_state;
  StringInstanceState string_instance_state;
  SystemInstanceState system_instance_state;
  ErrorHandlingState error_handling_instance_state;
  KmlInstanceState kml_instance_state;

  list<kphp::coro::task<>> shutdown_functions;

private:
  kphp::coro::task<> init_cli_instance() noexcept;
  kphp::coro::task<> init_server_instance() noexcept;
  kphp::coro::task<> finalize_cli_instance() noexcept;
  kphp::coro::task<> finalize_server_instance() const noexcept;

  enum class shutdown_state : uint8_t { not_started, in_progress, finished };
  shutdown_state shutdown_state_{shutdown_state::not_started};

  enum image_kind image_kind_ { image_kind::invalid };
  enum instance_kind instance_kind_ { instance_kind::invalid };

  static constexpr auto INIT_INSTANCE_ALLOCATOR_SIZE = static_cast<size_t>(16U * 1024U * 1024U);
};
