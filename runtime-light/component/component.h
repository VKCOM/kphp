#pragma once

#include <csetjmp>
#include <queue>

#include "runtime-light/allocator/memory_resource/resource_allocator.h"
#include "runtime-light/context.h"
#include "runtime-light/core/globals/php-script-globals.h"
#include "runtime-light/core/kphp_core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/output_control.h"
#include "runtime-light/stdlib/superglobals.h"
#include "runtime-light/streams/streams.h"

struct ComponentState {
  template<typename Key, typename Value>
  using unordered_map = memory_resource::stl::unordered_map<Key, Value, memory_resource::unsynchronized_pool_resource>;
  template<typename T>
  using deque = memory_resource::stl::deque<T, memory_resource::unsynchronized_pool_resource>;

  ComponentState() :
    php_script_mutable_globals_singleton(script_allocator.memory_resource),
    opened_streams(unordered_map<uint64_t, StreamRuntimeStatus>::allocator_type{script_allocator.memory_resource}),
    awaiting_coroutines(unordered_map<uint64_t, std::coroutine_handle<>>::allocator_type{script_allocator.memory_resource}),
    incoming_pending_queries(deque<uint64_t>::allocator_type{script_allocator.memory_resource}){}

  ~ComponentState() = default;

  sigjmp_buf exit_tag;
  dl::ScriptAllocator script_allocator;
  task_t<void> k_main;
  Response response;
  string_buffer static_sb;
  PhpScriptMutableGlobals php_script_mutable_globals_singleton;

  PollStatus poll_status = PollStatus::PollBlocked;
  uint64_t standard_stream = 0;
  std::coroutine_handle<> standard_handle;

  unordered_map<uint64_t, StreamRuntimeStatus> opened_streams; // подумать про необходимость opened_streams. Объединить с awaiting_coroutines
  unordered_map<uint64_t, std::coroutine_handle<>> awaiting_coroutines;
  deque<uint64_t> incoming_pending_queries;
};
