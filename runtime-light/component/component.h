#pragma once

#include <csetjmp>
#include <queue>

#include "runtime-light/core/kphp_core.h"
#include "runtime-light/stdlib/output_control.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/context.h"
#include "runtime-light/stdlib/superglobals.h"
#include "runtime-light/streams/streams.h"

struct ComponentState {
  ComponentState() = default;
  ~ComponentState() = default;

  //todo do not use heap
  std::queue<uint64_t> pending_queries;
  std::unordered_map<uint64_t, StreamSuspendReason> processed_queries;
  std::unordered_map<uint64_t, std::coroutine_handle<>> queries_handlers;

  sigjmp_buf exit_tag;
  dl::ScriptAllocator script_allocator;
  task_t<void> k_main;
  Response response;
  Superglobals superglobals;

  PollStatus poll_status = PollStatus::PollBlocked;
  uint64_t standard_stream = 0;
  std::coroutine_handle<> standard_handle;
};
