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
using on_rpc_request_start_callback_t = std::function<void(int64_t, int64_t, int64_t, int64_t, double, bool)>;
using on_rpc_request_finish_callback_t = std::function<void(int64_t, int64_t, double, int64_t)>;
using on_job_request_start_callback_t = std::function<void(int64_t, const class_instance<C$KphpJobWorkerRequest>&, double, bool)>;
using on_job_request_finish_callback_t = std::function<void(int64_t, double, int64_t)>;

using ShutdownType = int64_t;
using on_shutdown_functions_start_callback_t = std::function<void(int64_t, ShutdownType)>;
using on_shutdown_functions_finish_callback_t = std::function<void()>;

extern on_fork_switch_callback_t on_fork_start;
extern on_fork_state_change_callback_t on_fork_finish;
extern on_fork_switch_callback_t on_fork_switch;
extern on_rpc_request_start_callback_t on_rpc_request_start;
extern on_rpc_request_finish_callback_t on_rpc_request_finish;
extern on_job_request_start_callback_t on_job_request_start;
extern on_job_request_finish_callback_t on_job_request_finish;
extern on_net_to_script_switch_callback_t on_net_to_script_ctx_switch;
extern on_shutdown_functions_start_callback_t on_shutdown_functions_start;
extern on_shutdown_functions_finish_callback_t on_shutdown_functions_finish;

struct tracing_binary_buffer {
  int capacity{0};
  int *buf{nullptr};
  int *pos{nullptr};

  void init_and_alloc();
  void alloc_if_not_enough(int reserve_bytes);
  void clear();

  int size_bytes() const { return (pos - buf) * 4; }

  void write_int32(int64_t v) { *pos++ = static_cast<int>(v); }
  void write_int64(int64_t v);
  void write_uint32(int64_t v) { *pos++ = static_cast<unsigned int>(v); }
  void write_float32(double v) { float f32 = static_cast<float>(v); *pos++ = *reinterpret_cast<int *>(&f32); }
  void write_float64(double v);
  void write_string(const string &v);
  void write_bool(bool v);
};

extern tracing_binary_buffer trace_binlog;

void init_tracing_lib();
void free_tracing_lib();

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
void f$register_kphp_shutdown_functions_callbacks(const kphp_tracing::on_shutdown_functions_start_callback_t &on_start,
                                                     const kphp_tracing::on_shutdown_functions_finish_callback_t &on_finish);

void f$kphp_tracing_init_binlog();
string f$kphp_tracing_get_binlog_as_hex_string();

void f$kphp_tracing_write_event_type(int64_t event_type, int64_t custom24bits);
inline void f$kphp_tracing_write_int32(int64_t v) { kphp_tracing::trace_binlog.write_int32(v); }
inline void f$kphp_tracing_write_int64(int64_t v) { kphp_tracing::trace_binlog.write_int64(v); }
inline void f$kphp_tracing_write_uint32(int64_t v) { kphp_tracing::trace_binlog.write_uint32(v); }
inline void f$kphp_tracing_write_float32(double v) { kphp_tracing::trace_binlog.write_float32(v); }
inline void f$kphp_tracing_write_float64(double v) { kphp_tracing::trace_binlog.write_float64(v); }
inline void f$kphp_tracing_write_string(const string &v) { kphp_tracing::trace_binlog.write_string(v); }
inline void f$kphp_tracing_write_bool(bool v) { kphp_tracing::trace_binlog.write_bool(v); }

