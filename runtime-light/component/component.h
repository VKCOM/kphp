// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <csetjmp>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/memory-resource/resource_allocator.h"
#include "runtime-common/core/memory-resource/unsynchronized_pool_resource.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/core/globals/php-script-globals.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/header.h"
#include "runtime-light/scheduler/scheduler.h"
#include "runtime-light/server/http/http-server-context.h"
#include "runtime-light/server/job-worker/job-worker-server-context.h"
#include "runtime-light/stdlib/crypto/crypto-context.h"
#include "runtime-light/stdlib/curl/curl-context.h"
#include "runtime-light/stdlib/file/file-stream-context.h"
#include "runtime-light/stdlib/fork/fork-context.h"
#include "runtime-light/stdlib/job-worker/job-worker-client-context.h"
#include "runtime-light/stdlib/output/output-buffer.h"
#include "runtime-light/stdlib/regex/regex-context.h"
#include "runtime-light/stdlib/rpc/rpc-context.h"
#include "runtime-light/stdlib/string/string-context.h"
#include "runtime-light/stdlib/system/system-context.h"

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

  InstanceState() noexcept
    : runtime_allocator(INIT_RUNTIME_ALLOCATOR_SIZE, 0)
    , scheduler(runtime_allocator.memory_resource)
    , fork_instance_state(runtime_allocator.memory_resource)
    , php_script_mutable_globals_singleton(runtime_allocator.memory_resource)
    , rpc_instance_state(runtime_allocator.memory_resource)
    , incoming_streams_(deque<uint64_t>::allocator_type{runtime_allocator.memory_resource})
    , opened_streams_(unordered_set<uint64_t>::allocator_type{runtime_allocator.memory_resource})
    , pending_updates_(unordered_set<uint64_t>::allocator_type{runtime_allocator.memory_resource}) {}

  ~InstanceState() = default;

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

  uint64_t set_timer(std::chrono::nanoseconds) noexcept;
  void release_stream(uint64_t) noexcept;
  void release_all_streams() noexcept;

  RuntimeAllocator runtime_allocator;

  CoroutineScheduler scheduler;
  ForkInstanceState fork_instance_state;
  PollStatus poll_status{PollStatus::PollReschedule};

  Response response;
  PhpScriptMutableGlobals php_script_mutable_globals_singleton;

  RuntimeContext runtime_context;
  RpcInstanceState rpc_instance_state;
  HttpServerInstanceState http_server_instance_state{};
  JobWorkerClientInstanceState job_worker_client_instance_state{};
  JobWorkerServerInstanceState job_worker_server_instance_state{};

  RegexInstanceState regex_instance_state{};
  CurlInstanceState curl_instance_state{};
  CryptoInstanceState crypto_instance_state{};
  StringInstanceState string_instance_state{};
  SystemInstanceState system_instance_state{};
  FileStreamInstanceState file_stream_instance_state{};

private:
  task_t<void> main_task_;

  ImageKind image_kind_{ImageKind::Invalid};
  uint64_t standard_stream_{INVALID_PLATFORM_DESCRIPTOR};
  deque<uint64_t> incoming_streams_;
  unordered_set<uint64_t> opened_streams_;
  unordered_set<uint64_t> pending_updates_;

  static constexpr auto INIT_RUNTIME_ALLOCATOR_SIZE = static_cast<size_t>(512U * 1024U); // 512KB
};
