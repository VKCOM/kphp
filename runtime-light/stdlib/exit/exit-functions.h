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

  // Sending a request can be canceled if the instance closes the stream before it will be sent by RPC component.
  // Wait for all results to guarantee that the requests are sent.
  auto& rpc_client_instance_st{RpcClientInstanceState::get()};
  while (!rpc_client_instance_st.response_awaiter_tasks.empty()) {
    const auto it_response_awaiter{rpc_client_instance_st.response_awaiter_tasks.begin()};
    const auto& [query_id, awaiter_task]{*it_response_awaiter};

    rpc_client_instance_st.response_awaiter_tasks.erase(it_response_awaiter);
    rpc_client_instance_st.response_fetcher_instances.erase(query_id);

    co_await kphp::forks::id_managed(awaiter_task);
  }

  k2::exit(static_cast<int32_t>(exit_code));
}

inline kphp::coro::task<> f$die(mixed v = 0) noexcept {
  co_await f$exit(std::move(v));
}
