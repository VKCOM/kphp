// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/kphp_tracing.h"

namespace kphp_tracing {

on_fork_switch_callback_t on_fork_start;
on_fork_state_change_callback_t on_fork_finish;
on_fork_switch_callback_t on_fork_switch;
on_rpc_request_start_callback_t on_rpc_request_start;
on_rpc_request_finish_callback_t on_rpc_request_finish;
on_job_request_start_callback_t on_job_request_start;
on_job_request_finish_callback_t on_job_request_finish;
on_net_to_script_switch_callback_t on_net_to_script_ctx_switch;

void clear_callbacks() {
  on_fork_start = {};
  on_fork_finish = {};
  on_fork_switch = {};
  on_rpc_request_start = {};
  on_rpc_request_finish = {};
  on_job_request_start = {};
  on_job_request_finish = {};
  on_net_to_script_ctx_switch = {};
}

}

void f$register_kphp_on_fork_callbacks(const kphp_tracing::on_fork_switch_callback_t &on_fork_start,
                                       const kphp_tracing::on_fork_state_change_callback_t &on_fork_finish,
                                       const kphp_tracing::on_fork_switch_callback_t &on_fork_switch) {
  kphp_tracing::on_fork_start = on_fork_start;
  kphp_tracing::on_fork_finish = on_fork_finish;
  kphp_tracing::on_fork_switch = on_fork_switch;
}

void f$register_kphp_on_rpc_query_callbacks(const kphp_tracing::on_rpc_request_start_callback_t &on_start,
                                            const kphp_tracing::on_rpc_request_finish_callback_t &on_finish) {
  kphp_tracing::on_rpc_request_start = on_start;
  kphp_tracing::on_rpc_request_finish = on_finish;
}

void f$register_kphp_on_job_worker_callbacks(const kphp_tracing::on_job_request_start_callback_t &on_start,
                                             const kphp_tracing::on_job_request_finish_callback_t &on_finish) {
  kphp_tracing::on_job_request_start = on_start;
  kphp_tracing::on_job_request_finish = on_finish;
}

void f$register_kphp_on_swapcontext_callbacks(const kphp_tracing::on_net_to_script_switch_callback_t &on_net_to_script_switch) {
  kphp_tracing::on_net_to_script_ctx_switch = on_net_to_script_switch;
}
