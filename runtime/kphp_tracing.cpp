// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/kphp_tracing.h"

namespace kphp_tracing {

on_wait_start_callback_t on_wait_start_callback;
on_fork_state_change_callback_t on_wait_finish_callback;
on_fork_switch_callback_t on_fork_start;
on_fork_state_change_callback_t on_fork_finish;
on_fork_switch_callback_t on_fork_switch_callback;
on_rpc_request_start_callback_t on_rpc_request_start;
on_rpc_request_finish_callback_t on_rpc_request_finish;
on_job_request_start_callback_t on_job_request_start;
on_job_request_finish_callback_t on_job_request_finish;
on_net_to_script_switch_callback_t on_net_to_script_ctx_switch;

void clear_callbacks() {
  on_wait_start_callback = {};
  on_wait_finish_callback = {};
  on_fork_switch_callback = {};
  on_fork_start = {};
  on_fork_finish = {};
  on_rpc_request_start = {};
  on_rpc_request_finish = {};
  on_job_request_start = {};
  on_job_request_finish = {};
  on_net_to_script_ctx_switch = {};
}

}

void f$kphp_tracing_register_on_wait_callbacks(const kphp_tracing::on_wait_start_callback_t &on_start,
                                               const kphp_tracing::on_fork_state_change_callback_t &on_finish) {
  kphp_tracing::on_wait_start_callback = on_start;
  kphp_tracing::on_wait_finish_callback = on_finish;
}

void f$kphp_tracing_register_on_fork_switch_callback(const kphp_tracing::on_fork_switch_callback_t &callback) {
  kphp_tracing::on_fork_switch_callback = callback;
}

void f$kphp_tracing_register_on_fork_callbacks(const kphp_tracing::on_fork_switch_callback_t &on_start,
                                               const kphp_tracing::on_fork_state_change_callback_t &on_finish) {
  kphp_tracing::on_fork_start = on_start;
  kphp_tracing::on_fork_finish = on_finish;
}

void f$kphp_tracing_register_on_kphp_rpc_request_callbacks(const kphp_tracing::on_rpc_request_start_callback_t &on_start,
                                                           const kphp_tracing::on_rpc_request_finish_callback_t &on_finish) {
  kphp_tracing::on_rpc_request_start = on_start;
  kphp_tracing::on_rpc_request_finish = on_finish;
}

void f$kphp_tracing_register_on_kphp_job_request_callbacks(const kphp_tracing::on_job_request_start_callback_t &on_start,
                                                           const kphp_tracing::on_job_request_finish_callback_t &on_finish) {
  kphp_tracing::on_job_request_start = on_start;
  kphp_tracing::on_job_request_finish = on_finish;
}

void f$kphp_tracing_register_on_net_to_script_switch_callback(const kphp_tracing::on_net_to_script_switch_callback_t &on_net_to_script_switch) {
  kphp_tracing::on_net_to_script_ctx_switch = on_net_to_script_switch;
}
