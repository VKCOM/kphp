// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <functional>

#include "runtime/kphp_core.h"

struct C$KphpJobWorkerRequest;

namespace runtime_injection {

using on_fork_callback_t = std::function<void(int64_t fork_id)>;

using on_rpc_query_start_callback_t = std::function<
  void(int64_t rpc_query_id, int64_t actor_port, int64_t tl_magic, int64_t bytes_sent, double start_timestamp, bool is_no_result)
>;
using on_rpc_query_finish_callback_t = std::function<
  void(int64_t rpc_query_id, int64_t bytes_recv_or_error_code, double duration_sec)
>;

using on_job_worker_start_callback_t = std::function<
  void(int64_t job_id, const class_instance<C$KphpJobWorkerRequest> &job, double start_timestamp, bool is_no_reply)
>;
using on_job_worker_finish_callback_t = std::function<
  void(int64_t job_id, int64_t error_code, double duration_sec)
>;

using on_net_to_script_switch_callback_t = std::function<void(double now_timestamp, double net_time_delta)>;

using on_shutdown_functions_start_callback_t = std::function<void(int64_t shutdown_functions_cnt, int64_t shutdown_type, double now_timestamp)>;
using on_shutdown_functions_finish_callback_t = std::function<void(double now_timestamp)>;

using on_tracing_vslice_start_callback_t = std::function<void(int64_t vslice_id, double start_timestamp)>;
using on_tracing_vslice_finish_callback_t = std::function<void(int64_t vslice_id, double end_timestamp, int64_t memory_used)>;

using on_curl_easy_callback_t = std::function<void(int64_t curl_handle)>;
using on_curl_multi_callback_t = std::function<void(int64_t multi_handle, int64_t curl_handle)>;

using on_external_program_start_callback_t = std::function<
  void(int64_t exec_id, int64_t exec_type, string command)
>;
using on_external_program_finish_callback_t = std::function<
  void(int64_t exec_id, int64_t exit_code, mixed output)
>;

extern on_fork_callback_t on_fork_start;
extern on_fork_callback_t on_fork_finish;
extern on_fork_callback_t on_fork_switch;
extern on_rpc_query_start_callback_t on_rpc_query_start;
extern on_rpc_query_finish_callback_t on_rpc_query_finish;
extern on_job_worker_start_callback_t on_job_worker_start;
extern on_job_worker_finish_callback_t on_job_worker_finish;
extern on_net_to_script_switch_callback_t on_net_to_script_ctx_switch;
extern on_shutdown_functions_start_callback_t on_shutdown_functions_start;
extern on_shutdown_functions_finish_callback_t on_shutdown_functions_finish;
extern on_tracing_vslice_start_callback_t on_tracing_vslice_start;
extern on_tracing_vslice_finish_callback_t on_tracing_vslice_finish;
extern on_curl_easy_callback_t on_curl_exec_start;
extern on_curl_easy_callback_t on_curl_exec_finish;
extern on_curl_multi_callback_t on_curl_multi_add_handle;
extern on_curl_multi_callback_t on_curl_multi_remove_handle;
extern on_external_program_start_callback_t on_external_program_start;
extern on_external_program_finish_callback_t on_external_program_finish;

template<typename F, typename ...Args>
void invoke_callback(F &f, Args &&... args) noexcept {
  if (f) {
    f(std::forward<Args>(args)...);
  }
}

void free_callbacks();

} // namespace runtime_injection


// TODO: ensure this callbacks never swap context
void f$register_kphp_on_fork_callbacks(const runtime_injection::on_fork_callback_t &on_fork_start,
                                       const runtime_injection::on_fork_callback_t &on_fork_finish,
                                       const runtime_injection::on_fork_callback_t &on_fork_switch);
void f$register_kphp_on_rpc_query_callbacks(const runtime_injection::on_rpc_query_start_callback_t &on_start,
                                            const runtime_injection::on_rpc_query_finish_callback_t &on_finish);
void f$register_kphp_on_job_worker_callbacks(const runtime_injection::on_job_worker_start_callback_t &on_start,
                                             const runtime_injection::on_job_worker_finish_callback_t &on_finish);
void f$register_kphp_on_swapcontext_callbacks(const runtime_injection::on_net_to_script_switch_callback_t &on_net_to_script_switch);
void f$register_kphp_on_shutdown_callbacks(const runtime_injection::on_shutdown_functions_start_callback_t &on_start,
                                           const runtime_injection::on_shutdown_functions_finish_callback_t &on_finish);
void f$register_kphp_on_tracing_vslice_callbacks(const runtime_injection::on_tracing_vslice_start_callback_t &on_start,
                                                 const runtime_injection::on_tracing_vslice_finish_callback_t &on_finish);
void f$register_kphp_on_curl_callbacks(const runtime_injection::on_curl_easy_callback_t &on_curl_exec_start,
                                       const runtime_injection::on_curl_easy_callback_t &on_curl_exec_finish,
                                       const runtime_injection::on_curl_multi_callback_t &on_curl_multi_add_handle,
                                       const runtime_injection::on_curl_multi_callback_t &on_curl_multi_remove_handle);
void f$register_kphp_on_extenral_program_execution_callbacks(const runtime_injection::on_external_program_start_callback_t &on_start,
                                                             const runtime_injection::on_external_program_finish_callback_t &on_finish);
