// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/runtime_injection.h"

namespace runtime_injection {

on_fork_callback_t on_fork_start;
on_fork_callback_t on_fork_finish;
on_fork_callback_t on_fork_switch;
on_rpc_query_start_callback_t on_rpc_query_start;
on_rpc_query_finish_callback_t on_rpc_query_finish;
on_job_worker_start_callback_t on_job_worker_start;
on_job_worker_finish_callback_t on_job_worker_finish;
on_net_to_script_switch_callback_t on_net_to_script_ctx_switch;
on_shutdown_functions_start_callback_t on_shutdown_functions_start;
on_shutdown_functions_finish_callback_t on_shutdown_functions_finish;
on_tracing_vslice_start_callback_t on_tracing_vslice_start;
on_tracing_vslice_finish_callback_t on_tracing_vslice_finish;
on_curl_easy_callback_t on_curl_exec_start;
on_curl_easy_callback_t on_curl_exec_finish;
on_curl_multi_callback_t on_curl_multi_add_handle;
on_curl_multi_callback_t on_curl_multi_remove_handle;
on_external_program_start_callback_t on_external_program_start;
on_external_program_finish_callback_t on_external_program_finish;

void free_callbacks() {
  on_fork_start = {};
  on_fork_finish = {};
  on_fork_switch = {};
  on_rpc_query_start = {};
  on_rpc_query_finish = {};
  on_job_worker_start = {};
  on_job_worker_finish = {};
  on_net_to_script_ctx_switch = {};
  on_shutdown_functions_start = {};
  on_shutdown_functions_finish = {};
  on_tracing_vslice_start = {};
  on_tracing_vslice_finish = {};
  on_curl_exec_start = {};
  on_curl_exec_finish = {};
  on_curl_multi_add_handle = {};
  on_curl_multi_remove_handle = {};
  on_external_program_start = {};
  on_external_program_finish = {};
}

} // namespace runtime_injection

void f$register_kphp_on_fork_callbacks(const runtime_injection::on_fork_callback_t &on_fork_start,
                                       const runtime_injection::on_fork_callback_t &on_fork_finish,
                                       const runtime_injection::on_fork_callback_t &on_fork_switch) {
  runtime_injection::on_fork_start = on_fork_start;
  runtime_injection::on_fork_finish = on_fork_finish;
  runtime_injection::on_fork_switch = on_fork_switch;
}

void f$register_kphp_on_rpc_query_callbacks(const runtime_injection::on_rpc_query_start_callback_t &on_start,
                                            const runtime_injection::on_rpc_query_finish_callback_t &on_finish) {
  runtime_injection::on_rpc_query_start = on_start;
  runtime_injection::on_rpc_query_finish = on_finish;
}

void f$register_kphp_on_job_worker_callbacks(const runtime_injection::on_job_worker_start_callback_t &on_start,
                                             const runtime_injection::on_job_worker_finish_callback_t &on_finish) {
  runtime_injection::on_job_worker_start = on_start;
  runtime_injection::on_job_worker_finish = on_finish;
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

void f$register_kphp_on_curl_callbacks(
  const runtime_injection::on_curl_easy_callback_t &on_curl_exec_start,
  const runtime_injection::on_curl_easy_callback_t &on_curl_exec_finish,
  const runtime_injection::on_curl_multi_callback_t &on_curl_multi_add_handle,
  const runtime_injection::on_curl_multi_callback_t &on_curl_multi_remove_handle
) {
  runtime_injection::on_curl_exec_start = on_curl_exec_start;
  runtime_injection::on_curl_exec_finish = on_curl_exec_finish;
  runtime_injection::on_curl_multi_add_handle = on_curl_multi_add_handle;
  runtime_injection::on_curl_multi_remove_handle = on_curl_multi_remove_handle;
}

void f$register_kphp_on_extenral_program_execution_callbacks(const runtime_injection::on_external_program_start_callback_t &on_start,
                                                             const runtime_injection::on_external_program_finish_callback_t &on_finish) {
  runtime_injection::on_external_program_start = on_start;
  runtime_injection::on_external_program_finish = on_finish;
}
