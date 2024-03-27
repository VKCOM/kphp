#pragma once

#include <csetjmp>


#include "runtime-light/allocator/memory_resource/unsynchronized_pool_resource.h"
#include "runtime-light/core/kphp_core.h"
#include "runtime-light/stdlib/output_control.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/context.h"


struct ComponentState {
  ComponentState() = default;
  ~ComponentState();

  sigjmp_buf panic_buffer;
  dl::ScriptAllocator script_allocator;
  task_t<void> k_main;
  Response response;

  PollStatus poll_status = PollStatus::PollBlocked;
  uint64_t awaited_stream = -1; // in the future it will be map sd -> coroutine_handle
  std::coroutine_handle<> suspend_point;
  uint64_t standard_stream;
};
