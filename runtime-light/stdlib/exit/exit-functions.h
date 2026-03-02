// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/stdlib/fork/fork-functions.h"
#include "runtime-light/stdlib/output/output-state.h"
#include "runtime-light/stdlib/rpc/rpc-client-state.h"

inline kphp::coro::task<> f$exit(mixed v = 0) noexcept { // TODO: make it synchronous
  auto& instance_st{InstanceState::get()};

  int64_t exit_code{};
  if (v.is_string()) {
    OutputInstanceState::get().output_buffers.current_buffer().get() << v;
  } else if (v.is_int()) {
    int64_t v_code{v.to_int()};
    exit_code = v_code >= 0 && v_code <= 254 ? v_code : 1; // valid PHP exit codes: [0..254]
  } else {
    exit_code = 1;
  }
  co_await kphp::forks::id_managed(instance_st.run_instance_epilogue());

  /*
   * Unlike regular RPC requests, whose results the user code waits for via rpc_fetch_responses, there by guaranteeing they are sent, the code does not wait for
   * requests sent with the ignore_answer flag. Therefore, we can’t guarantee that the coroutines responsible for sending ignore_answer requests have finished.
   * This means the requests might not be sent if the instance terminates.
   *
   * This await suspends the current coroutine until all pending ignore_answer requests are
   * fully sent. While suspended, other forks and coroutines may continue running.
   *
   * After this call completes, delivery of all ignore_answer requests is guaranteed.
   */
  co_await RpcClientInstanceState::get().ensure_ignore_answer_requests_sent();

  k2::exit(static_cast<int32_t>(exit_code));
}

inline kphp::coro::task<> f$die(mixed v = 0) noexcept {
  co_await f$exit(std::move(v));
}
