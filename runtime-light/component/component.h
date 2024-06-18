#pragma once

#include <coroutine>
#include <csetjmp>
#include <queue>
#include <functional>
#include <setjmp.h>

#include "runtime-light/allocator/memory_resource/resource_allocator.h"
#include "runtime-light/allocator/memory_resource/unsynchronized_pool_resource.h"
#include "runtime-light/core/globals/php-script-globals.h"
#include "runtime-light/core/kphp_core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/output_control.h"
#include "runtime-light/stdlib/superglobals.h"
#include "runtime-light/streams/streams.h"
#include "runtime-light/utils/context.h"
#include "runtime-light/coroutine/fork.h"

struct ComponentState {
  template<typename Key, typename Value>
  using unordered_map = memory_resource::stl::unordered_map<Key, Value, memory_resource::unsynchronized_pool_resource>;
  template<typename Value>
  using unordered_set = memory_resource::stl::unordered_set<Value, memory_resource::unsynchronized_pool_resource>;
  template<typename T>
  using deque = memory_resource::stl::deque<T, memory_resource::unsynchronized_pool_resource>;

  ComponentState() :
    kphp_fork_context(script_allocator.memory_resource),
    php_script_mutable_globals_singleton(script_allocator.memory_resource),
    opened_streams(unordered_set<uint64_t>::allocator_type{script_allocator.memory_resource}),
    timer_callbacks(unordered_map<uint64_t, std::function<void()>>::allocator_type{script_allocator.memory_resource}),
    incoming_pending_queries(deque<uint64_t>::allocator_type{script_allocator.memory_resource}){}

  ~ComponentState() = default;

  inline bool not_finished() const noexcept {
    return poll_status != PollStatus::PollFinishedOk && poll_status != PollStatus::PollFinishedError;
  }
  bool is_stream_already_being_processed(uint64_t stream_d);
  void process_new_input_stream(uint64_t stream_d);
  void init_script_execution();

  KphpForkContext kphp_fork_context;

  dl::ScriptAllocator script_allocator;
  Response response;
  PhpScriptMutableGlobals php_script_mutable_globals_singleton;

  PollStatus poll_status = PollStatus::PollReschedule;
  uint64_t standard_stream = 0;
  bool wait_incoming_stream = false;


  unordered_set<uint64_t> opened_streams; // подумать про необходимость opened_streams. Объединить с awaiting_coroutines
  unordered_map<uint64_t, std::function<void()>> timer_callbacks;
  deque<uint64_t> incoming_pending_queries;

private:
  bool is_stream_timer(uint64_t stream_d);

  void process_timer(uint64_t stream_d);
};
