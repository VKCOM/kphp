// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <coroutine>
#include <csetjmp>
#include <cstddef>
#include <functional>
#include <queue>

#include "runtime-core/memory-resource/resource_allocator.h"
#include "runtime-core/memory-resource/unsynchronized_pool_resource.h"
#include "runtime-core/runtime-core.h"

#include "runtime-light/core/globals/php-script-globals.h"
#include "runtime-light/coroutine/fork-context.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/output-control.h"
#include "runtime-light/stdlib/rpc/rpc-context.h"
#include "runtime-light/stdlib/superglobals.h"
#include "runtime-light/streams/streams.h"
#include "runtime-light/utils/context.h"

struct ComponentState {
  template<typename Key, typename Value>
  using unordered_map = memory_resource::stl::unordered_map<Key, Value, memory_resource::unsynchronized_pool_resource>;
  template<typename Value>
  using unordered_set = memory_resource::stl::unordered_set<Value, memory_resource::unsynchronized_pool_resource>;
  template<typename T>
  using deque = memory_resource::stl::deque<T, memory_resource::unsynchronized_pool_resource>;
  static constexpr auto INIT_RUNTIME_ALLOCATOR_SIZE = static_cast<size_t>(512U * 1024U); // 512KB

  ComponentState()
    : runtime_allocator(INIT_RUNTIME_ALLOCATOR_SIZE, 0)
    , kphp_fork_context(runtime_allocator.memory_resource)
    , rpc_component_context(runtime_allocator.memory_resource)
    , php_script_mutable_globals_singleton(runtime_allocator.memory_resource)
    , opened_descriptors(unordered_map<uint64_t, DescriptorRuntimeStatus>::allocator_type{runtime_allocator.memory_resource})
    , incoming_pending_queries(deque<uint64_t>::allocator_type{runtime_allocator.memory_resource}) {}

  ~ComponentState() = default;

  bool not_finished() const noexcept {
    return poll_status != PollStatus::PollFinishedOk && poll_status != PollStatus::PollFinishedError;
  }
  bool is_descriptor_stream(uint64_t update_d);
  bool is_descriptor_timer(uint64_t update_d);
  void process_new_input_stream(uint64_t stream_d);
  void init_script_execution();

  RuntimeAllocator runtime_allocator;
  KphpCoreContext kphp_core_context;
  KphpForkContext kphp_fork_context;
  RpcComponentContext rpc_component_context;
  Response response;
  PhpScriptMutableGlobals php_script_mutable_globals_singleton;

  PollStatus poll_status = PollStatus::PollReschedule;

  unordered_map<uint64_t, DescriptorRuntimeStatus> opened_descriptors;

  uint64_t standard_stream = 0;
  deque<uint64_t> incoming_pending_queries;
};
