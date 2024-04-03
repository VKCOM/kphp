#pragma once

#include <csetjmp>

#include "runtime-light/core/kphp_core.h"
#include "runtime-light/stdlib/output_control.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/coroutine/stack.h"
#include "runtime-light/context.h"
#include "runtime-light/stdlib/superglobals.h"

enum ComponentStatus {
  ComponentInited,
  ComponentRunning,
};

struct ComponentState {
  ComponentState() = default;
  ~ComponentState() = default;

  sigjmp_buf exit_tag;
  dl::ScriptAllocator script_allocator;
  CoroutineStack coroutine_stack;
  task_t<void> k_main;
  Response response;
  Superglobals superglobals;

  ComponentStatus component_status = ComponentInited;
  PollStatus poll_status = PollStatus::PollBlocked;
  uint64_t awaited_stream = -1; // in the future it will be map sd -> coroutine_handle
  std::coroutine_handle<> suspend_point;
  uint64_t standard_stream;
};
