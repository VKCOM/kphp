#pragma once

#include <coroutine>
#include <csetjmp>
#include <queue>
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

struct ComponentState {
  template<typename Key, typename Value>
  using unordered_map = memory_resource::stl::unordered_map<Key, Value, memory_resource::unsynchronized_pool_resource>;
  template<typename T>
  using deque = memory_resource::stl::deque<T, memory_resource::unsynchronized_pool_resource>;

  ComponentState() :
    php_script_mutable_globals_singleton(script_allocator.memory_resource),
    opened_streams(unordered_map<uint64_t, StreamRuntimeStatus>::allocator_type{script_allocator.memory_resource}),
    awaiting_coroutines(unordered_map<uint64_t, std::coroutine_handle<>>::allocator_type{script_allocator.memory_resource}),
    timer_callbacks(unordered_map<uint64_t, std::function<void()>>::allocator_type{script_allocator.memory_resource}),
    incoming_pending_queries(deque<uint64_t>::allocator_type{script_allocator.memory_resource}){}

  ~ComponentState() = default;

  inline bool not_finished() const noexcept {
    return poll_status != PollStatus::PollFinishedOk && poll_status != PollStatus::PollFinishedError;
  }

  void resume_if_was_rescheduled();

  bool is_stream_already_being_processed(uint64_t stream_d);

  void resume_if_wait_stream(uint64_t stream_d, StreamStatus status);

  void process_new_stream(uint64_t stream_d);


  sigjmp_buf exit_tag;
  dl::ScriptAllocator script_allocator;
  task_t<void> k_main;
  Response response;
  string_buffer static_sb;
  PhpScriptMutableGlobals php_script_mutable_globals_singleton;

  PollStatus poll_status = PollStatus::PollBlocked;
  uint64_t standard_stream = 0;
  std::coroutine_handle<> main_thread;

  unordered_map<uint64_t, StreamRuntimeStatus> opened_streams; // подумать про необходимость opened_streams. Объединить с awaiting_coroutines
  unordered_map<uint64_t, std::coroutine_handle<>> awaiting_coroutines;
  unordered_map<uint64_t, std::function<void()>> timer_callbacks;
  deque<uint64_t> incoming_pending_queries;

private:
  bool is_stream_timer(uint64_t stream_d);

  void process_timer(uint64_t stream_d);

  void process_stream(uint64_t stream_d, StreamStatus status);
};
