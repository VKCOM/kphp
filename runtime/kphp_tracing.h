// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <functional>

#include "runtime/kphp_core.h"

struct C$KphpJobWorkerRequest;

namespace kphp_tracing {

using on_net_to_script_switch_callback_t = std::function<void(double)>;
using on_fork_state_change_callback_t = std::function<void(int64_t)>;
using on_fork_switch_callback_t = std::function<void(int64_t, int64_t)>;
using on_rpc_request_start_callback_t = std::function<void(int64_t, int64_t, int64_t, int64_t, double)>;
using on_rpc_request_finish_callback_t = std::function<void(int64_t, int64_t, double, int64_t)>;
using on_job_request_start_callback_t = std::function<void(int64_t, const class_instance<C$KphpJobWorkerRequest>&)>;
using on_job_request_finish_callback_t = std::function<void(int64_t, double, int64_t)>;

extern on_fork_switch_callback_t on_fork_start;
extern on_fork_state_change_callback_t on_fork_finish;
extern on_fork_switch_callback_t on_fork_switch;
extern on_rpc_request_start_callback_t on_rpc_request_start;
extern on_rpc_request_finish_callback_t on_rpc_request_finish;
extern on_job_request_start_callback_t on_job_request_start;
extern on_job_request_finish_callback_t on_job_request_finish;
extern on_net_to_script_switch_callback_t on_net_to_script_ctx_switch;

void clear_callbacks();

}

// TODO: ensure this callbacks never swap context
void f$register_kphp_on_fork_callbacks(const kphp_tracing::on_fork_switch_callback_t &on_fork_start,
                                       const kphp_tracing::on_fork_state_change_callback_t &on_fork_finish,
                                       const kphp_tracing::on_fork_switch_callback_t &on_fork_switch);
void f$register_kphp_on_rpc_query_callbacks(const kphp_tracing::on_rpc_request_start_callback_t &on_start,
                                            const kphp_tracing::on_rpc_request_finish_callback_t &on_finish);
void f$register_kphp_on_job_worker_callbacks(const kphp_tracing::on_job_request_start_callback_t &on_start,
                                             const kphp_tracing::on_job_request_finish_callback_t &on_finish);
void f$register_kphp_on_swapcontext_callbacks(const kphp_tracing::on_net_to_script_switch_callback_t &on_net_to_script_switch);

