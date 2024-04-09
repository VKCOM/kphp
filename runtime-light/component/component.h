#pragma once

#include <csetjmp>
#include <queue>

#include "runtime-light/core/kphp_core.h"
#include "runtime-light/stdlib/output_control.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/context.h"
#include "runtime-light/stdlib/superglobals.h"
#include "runtime-light/streams/streams.h"
#include "runtime-light/allocator/memory_resource/resource_allocator.h"

struct ComponentState {
  template<typename Key, typename Value>
  using unordered_map = memory_resource::stl::unordered_map<Key, Value, memory_resource::unsynchronized_pool_resource>;
  template<typename T>
  using deque = memory_resource::stl::deque<T, memory_resource::unsynchronized_pool_resource>;

  ComponentState() :
    processed_queries(unordered_map<uint64_t, StreamSuspendReason>::allocator_type{script_allocator.memory_resource}),
    queries_handlers(unordered_map<uint64_t, std::coroutine_handle<>>::allocator_type{script_allocator.memory_resource}),
    pending_queries(deque<uint64_t>::allocator_type{script_allocator.memory_resource}){}

  ~ComponentState() = default;

  sigjmp_buf exit_tag;
  dl::ScriptAllocator script_allocator;
  task_t<void> k_main;
  Response response;
  Superglobals superglobals;

  PollStatus poll_status = PollStatus::PollBlocked;
  uint64_t standard_stream = 0;
  std::coroutine_handle<> standard_handle;

  unordered_map<uint64_t, StreamSuspendReason> processed_queries;
  unordered_map<uint64_t, std::coroutine_handle<>> queries_handlers;
  deque<uint64_t> pending_queries;
};
