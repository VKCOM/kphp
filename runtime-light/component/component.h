// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <csetjmp>
#include <cstddef>
#include <cstdint>

#include "runtime-core/memory-resource/resource_allocator.h"
#include "runtime-core/memory-resource/unsynchronized_pool_resource.h"
#include "runtime-core/runtime-core.h"
#include "runtime-light/core/globals/php-script-globals.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/header.h"
#include "runtime-light/scheduler/scheduler.h"
#include "runtime-light/stdlib/fork/fork-context.h"
#include "runtime-light/stdlib/output-control.h"
#include "runtime-light/stdlib/rpc/rpc-context.h"

constexpr uint64_t INVALID_PLATFORM_DESCRIPTOR = 0;

// Coroutine scheduler type. Change it here if you want to use another scheduler
using CoroutineScheduler = SimpleCoroutineScheduler;
static_assert(CoroutineSchedulerConcept<CoroutineScheduler>);

struct ComponentState {
  template<typename T>
  using unordered_set = memory_resource::stl::unordered_set<T, memory_resource::unsynchronized_pool_resource>;

  template<typename T>
  using deque = memory_resource::stl::deque<T, memory_resource::unsynchronized_pool_resource>;

  ComponentState() noexcept
    : runtime_allocator(INIT_RUNTIME_ALLOCATOR_SIZE, 0)
    , scheduler(runtime_allocator.memory_resource)
    , fork_component_context(runtime_allocator.memory_resource)
    , php_script_mutable_globals_singleton(runtime_allocator.memory_resource)
    , rpc_component_context(runtime_allocator.memory_resource)
    , incoming_streams_(deque<uint64_t>::allocator_type{runtime_allocator.memory_resource})
    , opened_streams_(unordered_set<uint64_t>::allocator_type{runtime_allocator.memory_resource})
    , pending_updates_(unordered_set<uint64_t>::allocator_type{runtime_allocator.memory_resource}) {}

  ~ComponentState() = default;

  void init_script_execution() noexcept;
  void process_platform_updates() noexcept;

  bool stream_updated(uint64_t) const noexcept;
  const unordered_set<uint64_t> &opened_streams() const noexcept;
  const deque<uint64_t> &incoming_streams() const noexcept;
  uint64_t standard_stream() const noexcept;
  uint64_t take_incoming_stream() noexcept;
  uint64_t open_stream(const string &) noexcept;
  uint64_t set_timer(std::chrono::nanoseconds) noexcept;
  void release_stream(uint64_t) noexcept;
  void release_all_streams() noexcept;

  RuntimeAllocator runtime_allocator;

  CoroutineScheduler scheduler;
  ForkComponentContext fork_component_context;
  PollStatus poll_status = PollStatus::PollReschedule;

  Response response;
  PhpScriptMutableGlobals php_script_mutable_globals_singleton;

  KphpCoreContext kphp_core_context;
  RpcComponentContext rpc_component_context;

private:
  task_t<void> main_task;

  uint64_t standard_stream_{INVALID_PLATFORM_DESCRIPTOR};
  deque<uint64_t> incoming_streams_;
  unordered_set<uint64_t> opened_streams_;
  unordered_set<uint64_t> pending_updates_;

  static constexpr auto INIT_RUNTIME_ALLOCATOR_SIZE = static_cast<size_t>(512U * 1024U); // 512KB
};
