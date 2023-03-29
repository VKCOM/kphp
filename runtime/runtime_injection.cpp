// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/runtime_injection.h"

namespace runtime_injection {

on_fork_start_callback_t on_fork_start;
on_fork_finish_callback_t on_fork_finish;
on_fork_switch_callback_t on_fork_switch;
on_rpc_request_start_callback_t on_rpc_request_start;
on_rpc_request_finish_callback_t on_rpc_request_finish;
on_job_request_start_callback_t on_job_request_start;
on_job_request_finish_callback_t on_job_request_finish;
on_net_to_script_switch_callback_t on_net_to_script_ctx_switch;
on_shutdown_functions_start_callback_t on_shutdown_functions_start;
on_shutdown_functions_finish_callback_t on_shutdown_functions_finish;
on_tracing_vslice_start_callback_t on_tracing_vslice_start;
on_tracing_vslice_finish_callback_t on_tracing_vslice_finish;

void free_callbacks() {
  on_fork_start = {};
  on_fork_finish = {};
  on_fork_switch = {};
  on_rpc_request_start = {};
  on_rpc_request_finish = {};
  on_job_request_start = {};
  on_job_request_finish = {};
  on_net_to_script_ctx_switch = {};
  on_shutdown_functions_start = {};
  on_shutdown_functions_finish = {};
  on_tracing_vslice_start = {};
  on_tracing_vslice_finish = {};
}

} // namespace runtime_injection

void f$register_kphp_on_fork_callbacks(const runtime_injection::on_fork_start_callback_t &on_fork_start,
                                       const runtime_injection::on_fork_finish_callback_t &on_fork_finish,
                                       const runtime_injection::on_fork_switch_callback_t &on_fork_switch) {
  runtime_injection::on_fork_start = on_fork_start;
  runtime_injection::on_fork_finish = on_fork_finish;
  runtime_injection::on_fork_switch = on_fork_switch;
}

void f$register_kphp_on_rpc_query_callbacks(const runtime_injection::on_rpc_request_start_callback_t &on_start,
                                            const runtime_injection::on_rpc_request_finish_callback_t &on_finish) {
  runtime_injection::on_rpc_request_start = on_start;
  runtime_injection::on_rpc_request_finish = on_finish;
}

void f$register_kphp_on_job_worker_callbacks(const runtime_injection::on_job_request_start_callback_t &on_start,
                                             const runtime_injection::on_job_request_finish_callback_t &on_finish) {
  runtime_injection::on_job_request_start = on_start;
  runtime_injection::on_job_request_finish = on_finish;
}

void f$register_kphp_on_swapcontext_callbacks(const runtime_injection::on_net_to_script_switch_callback_t &on_net_to_script_switch) {
  runtime_injection::on_net_to_script_ctx_switch = on_net_to_script_switch;
}

void f$register_kphp_on_shutdown_callbacks(const runtime_injection::on_shutdown_functions_start_callback_t &on_start,
                                           const runtime_injection::on_shutdown_functions_finish_callback_t &on_finish) {
  runtime_injection::on_shutdown_functions_start = on_start;
  runtime_injection::on_shutdown_functions_finish = on_finish;
}

void f$register_kphp_on_tracing_vslice_callbacks(const runtime_injection::on_tracing_vslice_start_callback_t &on_start,
                                                 const runtime_injection::on_tracing_vslice_finish_callback_t &on_finish) {
  runtime_injection::on_tracing_vslice_start = on_start;
  runtime_injection::on_tracing_vslice_finish = on_finish;
}
