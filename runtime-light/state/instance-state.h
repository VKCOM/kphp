// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/memory-resource/resource_allocator.h"
#include "runtime-common/core/memory-resource/unsynchronized_pool_resource.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/core/globals/php-script-globals.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/scheduler/scheduler.h"
#include "runtime-light/server/http/http-server-state.h"
#include "runtime-light/server/job-worker/job-worker-server-state.h"
#include "runtime-light/stdlib/crypto/crypto-state.h"
#include "runtime-light/stdlib/curl/curl-state.h"
#include "runtime-light/stdlib/file/file-stream-state.h"
#include "runtime-light/stdlib/fork/fork-state.h"
#include "runtime-light/stdlib/job-worker/job-worker-client-state.h"
#include "runtime-light/stdlib/math/random-state.h"
#include "runtime-light/stdlib/output/output-buffer.h"
#include "runtime-light/stdlib/rpc/rpc-state.h"
#include "runtime-light/stdlib/serialization/serialization-state.h"
#include "runtime-light/stdlib/string/regex-state.h"
#include "runtime-light/stdlib/string/string-state.h"
#include "runtime-light/stdlib/system/system-state.h"

inline constexpr uint64_t INVALID_PLATFORM_DESCRIPTOR = 0;

// Coroutine scheduler type. Change it here if you want to use another scheduler
using CoroutineScheduler = SimpleCoroutineScheduler;
static_assert(CoroutineSchedulerConcept<CoroutineScheduler>);

/**
 * Supported kinds of KPHP images:
 * 1. CLI — works the same way as regular PHP script does
 * 2. Server — automatically accepts a stream and expects it to contain either http or job worker request
 * 3. Oneshot — can only accept one incoming stream
 * 4. Multishot — can accept any number of incoming streams
 */
enum class ImageKind : uint8_t { Invalid, CLI, Server, Oneshot, Multishot };

struct InstanceState final : vk::not_copyable {
  template<typename T>
  using unordered_set = memory_resource::stl::unordered_set<T, memory_resource::unsynchronized_pool_resource>;

  template<typename T>
  using deque = memory_resource::stl::deque<T, memory_resource::unsynchronized_pool_resource>;

  template<typename T>
  using list = memory_resource::stl::list<T, memory_resource::unsynchronized_pool_resource>;

  InstanceState() noexcept
    : allocator(INIT_INSTANCE_ALLOCATOR_SIZE, 0)
    , scheduler(allocator.memory_resource)
    , fork_instance_state(allocator.memory_resource)
    , php_script_mutable_globals_singleton(allocator.memory_resource)
    , rpc_instance_state(allocator.memory_resource)
    , http_server_instance_state(allocator.memory_resource)
    , regex_instance_state(allocator.memory_resource)
    , shutdown_functions(decltype(shutdown_functions)::allocator_type{allocator.memory_resource})
    , incoming_streams_(decltype(incoming_streams_)::allocator_type{allocator.memory_resource})
    , opened_streams_(decltype(opened_streams_)::allocator_type{allocator.memory_resource})
    , pending_updates_(decltype(pending_updates_)::allocator_type{allocator.memory_resource}) {}

  ~InstanceState() = default;

  static InstanceState &get() noexcept {
    return *k2::instance_state();
  }

  void init_script_execution() noexcept;

  template<ImageKind>
  task_t<void> run_instance_prologue() noexcept;

  task_t<void> run_instance_epilogue() noexcept;

  ImageKind image_kind() const noexcept {
    return image_kind_;
  }

  void process_platform_updates() noexcept;

  bool stream_updated(uint64_t stream_d) const noexcept {
    return pending_updates_.contains(stream_d);
  }
  const unordered_set<uint64_t> &opened_streams() const noexcept {
    return opened_streams_;
  }
  const deque<uint64_t> &incoming_streams() const noexcept {
    return incoming_streams_;
  }
  uint64_t standard_stream() const noexcept {
    return standard_stream_;
  }
  uint64_t take_incoming_stream() noexcept;

  uint64_t open_stream(std::string_view) noexcept;
  uint64_t open_stream(const string &component_name) noexcept {
    return open_stream(std::string_view{component_name.c_str(), static_cast<size_t>(component_name.size())});
  }

  struct ConnectionResult {
    uint64_t stream_d{};
    int32_t error_code{};
  };

  template<typename Connector>
  requires(std::invocable<Connector, uint64_t *, const char *, size_t>
             &&std::is_same_v<std::invoke_result_t<Connector, uint64_t *, const char *, size_t>, int32_t>) ConnectionResult
  connect_to(const std::string_view &host, Connector &&connector) noexcept {
    uint64_t stream_d{0};
    const int32_t error_number{std::invoke(connector, std::addressof(stream_d), host.data(), host.size())};

    if (error_number != k2::errno_ok) {
      php_warning("Cannot connect to host %s, errno %d", host.data(), error_number);
      return {INVALID_PLATFORM_DESCRIPTOR, error_number};
    }
    opened_streams_.insert(stream_d);
    php_debug("connect to %s on stream %" PRIu64, host.data(), stream_d);
    return {stream_d, error_number};
  }

  uint64_t set_timer(std::chrono::nanoseconds) noexcept;
  void release_stream(uint64_t) noexcept;
  void release_all_streams() noexcept;

  RuntimeAllocator allocator;

  CoroutineScheduler scheduler;
  ForkInstanceState fork_instance_state;
  k2::PollStatus poll_status{k2::PollStatus::PollReschedule};

  Response response;
  PhpScriptMutableGlobals php_script_mutable_globals_singleton;

  RuntimeContext runtime_context;
  RpcInstanceState rpc_instance_state;
  SerializationInstanceState serialization_instance_state;
  HttpServerInstanceState http_server_instance_state;
  JobWorkerClientInstanceState job_worker_client_instance_state{};
  JobWorkerServerInstanceState job_worker_server_instance_state{};

  RandomInstanceState random_instance_state;
  RegexInstanceState regex_instance_state;
  CurlInstanceState curl_instance_state{};
  CryptoInstanceState crypto_instance_state{};
  StringInstanceState string_instance_state{};
  SystemInstanceState system_instance_state{};
  FileStreamInstanceState file_stream_instance_state{};

  list<task_t<void>> shutdown_functions;

private:
  task_t<void> main_task_;

  enum class shutdown_state : uint8_t { not_started, in_progress, finished };
  shutdown_state shutdown_state_{shutdown_state::not_started};

  ImageKind image_kind_{ImageKind::Invalid};
  uint64_t standard_stream_{INVALID_PLATFORM_DESCRIPTOR};
  deque<uint64_t> incoming_streams_;
  unordered_set<uint64_t> opened_streams_;
  unordered_set<uint64_t> pending_updates_;

  static constexpr auto INIT_INSTANCE_ALLOCATOR_SIZE = static_cast<size_t>(512U * 1024U); // 512KB
};
