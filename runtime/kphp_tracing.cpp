// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/kphp_tracing.h"
#include "runtime/kphp_tracing_binlog.h"

#include <chrono>

#include "runtime/critical_section.h"
#include "runtime/job-workers/job-interface.h"
#include "runtime/interface.h"
#include "runtime/math_functions.h"
#include "runtime/resumable.h"
#include "runtime/string_functions.h"
#include "runtime/tl/rpc_function.h"
#include "runtime/tl/tl_magics_decoding.h"

// KPHP tracing is a technology that aims to collect richer variety of data compared to Open telemetry
// and faster compared to existing tracing SDKs.
// In fact, every single request is traced in-memory with zero overhead, and probably flushed to disk on finish
// (if something interesting happens during a script execution, we have its trace — for any given request).
//
// As opposed to Open telemetry, KPHP does not store any state of spans: it does not have a Span class
// containing start/finish timestamps, name, parent, children, attributes, links, etc.
// Instead, when any event occurs, it is written to a binary in-memory append-only log (kphp_tracing_binlog.h).
// That binlog is a compact representation throughout all trace timeline.
//
// For example, when php code calls `$span->addAttributeInt("user_id", $uid)`, we need to push the following:
// 1) event_type etSpanAddAttributeInt32 (1 byte) + span_id (24 bits max) = 4 bytes
// 2) if a string "user_id" is seen the first time, register it; every time after, just an index = 4 bytes
// 3) $uid = 4 bytes
// 4) current timestamp as float offset from trace start = 4 bytes
//
// For example, when a rpc query is sent, we push rpcQueryID, actor+port, tl magic, timestamp = 20 bytes.
// Every possible event has its representation in binlog:
// * memory info is passed (=> it's possible to calculate memory consumption of each span)
// * context switches from script to net (=> it's possible to calculate wait_time of each span)
// * forks start and switching (=> it's possible to track active span in a coroutine-based single-threaded model)
// * rpc queries, job workers, curl requests, exec, file i/o;
// * ... and much more.
//
// As a consequence, addAttribute() exists, but getAttribute() does not.
// startSpan() exists, but getParentSpan() does not. updateName() exists, but getName() does not.
//
// On script finish, binlog is probably flushed to a disk (if php callback returns true).
// Bytes are flushed as hex strings to "kphp_log.traces.jsonl" files, next to json logs.
//
// As a separate project, outside KPHP, there is a decoder written in Go that reads these files and replays traces.
// This functionality is embedded into kw-parser, that is not open-sourced yet.
// kw-parser watches for changes (in a way it does for json files), decodes traces, and pushes them to ClickHouse.
// Also, there is a separate UI where users can see saved traces and search for them via various criteria.
// Backend of this UI reads that ClickHouse table, frontend is written in React, also not open sourced.
// Hopefully, some time later, StatsHouse will become an endpoint that can consume, decode, store and visualize traces.
// Then KPHP will push buffers not into files, but into local StatsHouse directly, as well as metrics.
//
// Typically, PHP code calls `kphp_tracing_init()` at the very beginning.
// It turns tracing on with trace_level=1 (default mode).
// PHP code can later call `kphp_tracing_set_level(2)` to turn advanced mode.
// Then KPHP will track much more info (even memory allocations and every regexp).
// Default trace level has about 0.15% overhead.
// Advanced trace level has about 2% overhead and should be invoked very-very rarely.
//
// KPHP exposes low-level functions like `kphp_tracing_start_span()`, `kphp_tracing_register_on_finish()`, and others.
// In vk.com, we have the KphpTracing class wrapping low-level calls into a more friendly API
// considering Modulite and Sentry collaboration.
//
// The KPHP compiler also knows about tracing.
// Thus, if a php function is annotated with `@kphp-tracing`, KPHP codegenerates a span
// that automatically starts on function start and finishes on every return/throw/etc.
// There is even `@kphp-tracing aggregate` for functions that are called very often,
// and instead of seeing each invocation, we want to have total amount of calls and time spent.
// Aggregations are collected only on level 2.
//
// There are also some key points for binlog size reducing (remember, that every request is traced in memory).
// For instance, in UI we want to see "memcache.get", not 0x1278241,
// but transferring same strings every time is ridiculous.
// Instead, a map of int->string is registered once - it's called "enum".
// A KPHP worker, once, before flushing any trace, flushes all enums (rpc actors, tl functions).
// From decoder's point of view, registering enums is just a piece of binlog.
// Moreover, this approach is even more general.
// PHP code can register its own enums (for example, a list of countries, a list of colors, etc.),
// and then reference them via `$span->addAttributeEnum()`.
// In UI, the end user will see "Germany" instead of a number "23".


// a strong implementation is codegenerated by the compiler if any `@kphp-tracing aggregate` exists
// see compiler/code-gen/files/tracing-autogen.cpp
void __attribute__ ((weak)) kphp_tracing_autogen_func_call_strings(array<string> &out) noexcept;

namespace kphp_tracing {

constexpr int SPECIAL_SPAN_ID_ROOT = 1;
constexpr int SPECIAL_SPAN_ID_CURRENT_ACTIVE = 65535;

constexpr int SPECIAL_ENUM_FUNC_AGGREGATES = 1;
constexpr int SPECIAL_ENUM_TL_MAGICS = 65001;

static bool worker_process_has_sent_enums = false;

tracing_binary_buffer cur_binlog;
int cur_trace_level;
static int cur_last_span_id;
static double cur_start_timestamp;
static tracing_binary_buffer flush_strings_binlog;
static class_instance<C$KphpDiv> cur_trace;
static on_trace_finish_callback_t cur_on_finish_callback;
static on_trace_enums_callback_t cur_on_enums_callback;
static on_rpc_provide_details_typed_t cur_on_rpc_details_typed_callback;
static on_rpc_provide_details_untyped_t cur_on_rpc_details_untyped_callback;

int generate_uniq_id() {
  return f$mt_rand();
}

void init_tracing_lib() {
  cur_trace_level = -1;
  cur_last_span_id = 0;
  cur_start_timestamp = 0.0;
  flush_strings_binlog.set_use_heap_memory();
}

void free_tracing_lib() {
  cur_binlog.clear(false);
  cur_on_finish_callback = {};
  cur_on_enums_callback = {};
  cur_on_rpc_details_typed_callback = {};
  cur_on_rpc_details_untyped_callback = {};
  cur_trace = class_instance<C$KphpDiv>{};
  free_tracing_binlog_lib();
}

// fill the same enum as `@kphp-tracing aggregate` codegens -
// but for built-in functions that are traced
static void fill_BuiltinFuncID_strings(array<string> &out) noexcept {
  auto add = [&out](BuiltinFuncID func_id, const char *title) {
    out.set_value(static_cast<int64_t>(func_id), string(title, strlen(title)));
  };
  auto add_branch = [&out](BuiltinFuncID func_id, BuiltinFuncID branch_num, const char *title) {
    out.set_value(static_cast<int64_t>(func_id) + (static_cast<int64_t>(branch_num) << static_cast<int>(BuiltinFuncID::_shift_for_branch)), string(title, strlen(title)));
  };

  out.set_value(0, string("unknown", 7));

  add(BuiltinFuncID::preg_match, "preg_match");
  add_branch(BuiltinFuncID::preg_match, BuiltinFuncID::branch_regex_needs_compilation, "needs compilation");
  add(BuiltinFuncID::preg_match_all, "preg_match_all");
  add_branch(BuiltinFuncID::preg_match_all, BuiltinFuncID::branch_regex_needs_compilation, "needs compilation");
  add(BuiltinFuncID::preg_split, "preg_split");
  add_branch(BuiltinFuncID::preg_split, BuiltinFuncID::branch_regex_needs_compilation, "needs compilation");
  add(BuiltinFuncID::preg_replace, "preg_replace");
  add_branch(BuiltinFuncID::preg_replace, BuiltinFuncID::branch_regex_needs_compilation, "needs compilation");
  add(BuiltinFuncID::preg_replace_callback, "preg_replace_callback");
  add_branch(BuiltinFuncID::preg_replace_callback, BuiltinFuncID::branch_regex_needs_compilation, "needs compilation");

  add(BuiltinFuncID::exec, "exec");
  add(BuiltinFuncID::system, "system");
  add(BuiltinFuncID::passthru, "passthru");

  add(BuiltinFuncID::aggregate_regexp_functions, "/regexp/");
}

static inline double calc_now_timestamp() noexcept {
  return std::chrono::duration<double>{std::chrono::system_clock::now().time_since_epoch()}.count();
}

static inline float calc_time_offset(double now_timestamp) noexcept {
  return static_cast<float>(now_timestamp - kphp_tracing::cur_start_timestamp);
}

static inline int calc_coroutine_id(int64_t fork_id) noexcept {
  return fork_id > 0 ? static_cast<int>(fork_id - first_forked_resumable_id + 1) : 0;
}

[[gnu::noinline]] [[gnu::cold]] static void provide_advanced_mem_details() {
  const auto &stats = dl::get_script_memory_stats();
  size_t allocations_count = stats.total_allocations;
  if (allocations_count > (1 << 24) - 1) {
    allocations_count = (1 << 24) - 1;  // to fit custom24bits
  }
  BinlogWriter::provideMemAdvanced(stats.memory_used, allocations_count, stats.total_memory_allocated);
}

[[gnu::always_inline]] static inline void provide_mem_info() {
  BinlogWriter::provideMemUsed(dl::get_script_memory_stats().memory_used);
  if (unlikely(kphp_tracing::cur_trace_level >= 2)) {
    provide_advanced_mem_details();
  }
}

static inline uint64_t encode_trace_context_int2(int div_id, int trace_flags) {
  return (static_cast<uint64_t>(div_id) << 32) + trace_flags;
}

void on_fork_start(int64_t fork_id) {
  double now_timestamp = calc_now_timestamp();
  BinlogWriter::onCoroutineStarted(calc_coroutine_id(fork_id), calc_time_offset(now_timestamp));
}

void on_fork_finish(int64_t fork_id) {
  double now_timestamp = calc_now_timestamp();
  BinlogWriter::onCoroutineFinished(calc_coroutine_id(fork_id), calc_time_offset(now_timestamp));
}

void on_fork_switch(int64_t fork_id) {
  provide_mem_info();
  BinlogWriter::onCoroutineSwitchTo(calc_coroutine_id(fork_id));
}

void on_fork_provide_name(int64_t fork_id, const string &fork_typeid) {
  string desc = fork_typeid;
  if (size_t pos = desc.find(string("c$")); pos != string::npos) {
    desc = desc.substr(pos + 2, desc.size() - pos - 2);
  }
  BinlogWriter::onCoroutineProvideDesc(calc_coroutine_id(fork_id), desc);
}

void on_rpc_query_send(int rpc_query_id, int actor_or_port, unsigned int tl_magic, int bytes_sent, double start_timestamp, bool is_no_result) {
  BinlogWriter::onRpcQuerySend(rpc_query_id, actor_or_port, tl_magic, bytes_sent, calc_time_offset(start_timestamp), is_no_result);
}

void on_rpc_query_finish(int rpc_query_id, int bytes_recv) {
  double now_timestamp = calc_now_timestamp();
  BinlogWriter::onRpcQueryGotResponse(rpc_query_id, bytes_recv, calc_time_offset(now_timestamp));
}

void on_rpc_query_fail(int rpc_query_id, int error_code) {
  double now_timestamp = calc_now_timestamp();
  BinlogWriter::onRpcQueryFailed(rpc_query_id, -error_code, calc_time_offset(now_timestamp));
}

void on_rpc_query_provide_details_after_send(const class_instance<C$VK$TL$RpcFunction> &typed_f, const mixed &untyped_obj) {
  if (!typed_f.is_null() && cur_on_rpc_details_typed_callback) {
    string details = cur_on_rpc_details_typed_callback(typed_f);
    BinlogWriter::onRpcQueryProvideDetails(details, true);
  } else if (!untyped_obj.is_null() && cur_on_rpc_details_untyped_callback) {
    string details = cur_on_rpc_details_untyped_callback(untyped_obj);
    BinlogWriter::onRpcQueryProvideDetails(details, false);
  }
}

void on_job_worker_start(int job_id, const string &job_class_name, double start_timestamp, bool is_no_reply) {
  BinlogWriter::onJobWorkerLaunch(job_id, job_class_name, calc_time_offset(start_timestamp), is_no_reply);
}

void on_job_worker_finish(int job_id) {
  double now_timestamp = calc_now_timestamp();
  BinlogWriter::onJobWorkerGotResponse(job_id, calc_time_offset(now_timestamp));
}

void on_job_worker_fail(int job_id, int error_code) {
  double now_timestamp = calc_now_timestamp();
  BinlogWriter::onJobWorkerFailed(job_id, -error_code, calc_time_offset(now_timestamp));
}

void on_curl_exec_start(int curl_handle, const string &url) {
  double now_timestamp = calc_now_timestamp();
  BinlogWriter::onCurlStarted(curl_handle, url, calc_time_offset(now_timestamp));
}

void on_curl_exec_finish(int curl_handle, int bytes_recv) {
  double now_timestamp = calc_now_timestamp();
  BinlogWriter::onCurlFinished(curl_handle, bytes_recv, calc_time_offset(now_timestamp));
}

void on_curl_exec_fail(int curl_handle, int error_code) {
  double now_timestamp = calc_now_timestamp();
  BinlogWriter::onCurlFailed(curl_handle, -error_code, calc_time_offset(now_timestamp));
}

void on_curl_add_attribute(int curl_handle, const string &key, const string &value) {
  BinlogWriter::onCurlAddedAttributeString(curl_handle, key, value);
}

void on_curl_add_attribute(int curl_handle, const string &key, int value) {
  BinlogWriter::onCurlAddedAttributeInt32(curl_handle, key, value);
}

void on_curl_multi_init(int multi_handle) {
  double now_timestamp = calc_now_timestamp();
  BinlogWriter::onCurlMultiStarted(multi_handle, calc_time_offset(now_timestamp));
}

void on_curl_multi_add_handle(int multi_handle, int curl_handle, const string &url) {
  double now_timestamp = calc_now_timestamp();
  BinlogWriter::onCurlMultiAddHandle(multi_handle, curl_handle, url, calc_time_offset(now_timestamp));
}

void on_curl_multi_remove_handle(int multi_handle, int curl_handle, int bytes_recv) {
  double now_timestamp = calc_now_timestamp();
  BinlogWriter::onCurlMultiRemoveHandle(multi_handle, curl_handle, bytes_recv, calc_time_offset(now_timestamp));
}

void on_curl_multi_close(int multi_handle) {
  double now_timestamp = calc_now_timestamp();
  BinlogWriter::onCurlMultiFinished(multi_handle, calc_time_offset(now_timestamp));
}

void on_external_program_start(int exec_id, BuiltinFuncID func_id, const string &command) {
  double now_timestamp = calc_now_timestamp();
  BinlogWriter::onExternalProgramStarted(exec_id, static_cast<int>(func_id), command, calc_time_offset(now_timestamp));
}

void on_external_program_finish(int exec_id, bool success, int64_t exit_code) {
  int error_code = success ? (exit_code == 127 ? -1 : 0) : -2;
  double now_timestamp = calc_now_timestamp();
  if (!error_code) {
    BinlogWriter::onExternalProgramFinished(exec_id, exit_code, calc_time_offset(now_timestamp));
  } else {
    BinlogWriter::onExternalProgramFailed(exec_id, -error_code, calc_time_offset(now_timestamp));
  }
}

void on_file_io_start(int fd, const string &file_name, bool is_write) {
  double now_timestamp = calc_now_timestamp();
  BinlogWriter::onFileIOStarted(fd, is_write, file_name, calc_time_offset(now_timestamp));
}

void on_file_io_finish(int fd, int bytes) {
  double now_timestamp = calc_now_timestamp();
  BinlogWriter::onFileIOFinished(fd, bytes, calc_time_offset(now_timestamp));
}

void on_file_io_fail(int fd, int error_code) {
  double now_timestamp = calc_now_timestamp();
  BinlogWriter::onFileIOFailed(fd, -error_code, calc_time_offset(now_timestamp));
}

void on_switch_context_to_script(double net_time_delta) {
  // net_time_delta is double (0.234 means "234 milliseconds")
  // to make binlog smaller, pass microseconds, they will fit 24 bit
  int microseconds = static_cast<int>(net_time_delta * 1000000);
  BinlogWriter::onWaitNet(microseconds);
}

void on_shutdown_functions_start(int /* ShutdownType */ shutdown_type_enum) {
  double now_timestamp = calc_now_timestamp();
  provide_mem_info();
  bool is_regular_shutdown = shutdown_type_enum == static_cast<int>(ShutdownType::normal);
  BinlogWriter::onScriptShuttingDown(calc_time_offset(now_timestamp), is_regular_shutdown);
}

void on_php_script_warning(const string &warning_message) {
  BinlogWriter::provideRuntimeWarning(0, warning_message);
}

// this function is called after shutdown functions have been executed (probably, after timeout)
// what we do here:
// 1) invoke php callback — it returns whether we should flush this trace or not
//    (we are in script context, so php callback may write stats via rpc requests)
// 2) if yes, binlog is flushed as well as all strings accumulated in "flush_strings_binlog" since the last flush
//    (if it's the first time after worker launch, all enums are flushed also)
// 3) if no, save newly-created binlog strings, see 'else' branch below
void on_php_script_finish_ok(double net_time, double script_time) {
  // note, that this function is called always, even if tracing is turned off,
  // because probably it was on, but turned off in the middle of execution, which is also a point of interest
  double now_timestamp = calc_now_timestamp();

  bool trace_needs_flush = false;
  if (!cur_trace.is_null() && cur_on_finish_callback && cur_trace_level >= 1) {
    cur_trace->end_timestamp = now_timestamp;
    trace_needs_flush = cur_on_finish_callback(now_timestamp);
  }
  char json_buf[1024];

  if (trace_needs_flush) {
    cur_trace->assign_random_trace_id_if_empty();
    const auto &webserver_stats = f$get_webserver_stats();
    provide_mem_info();
    BinlogWriter::onSpanAddedAttributeFloat32(SPECIAL_SPAN_ID_ROOT, string("kphp.net_time", 13), static_cast<float>(net_time));
    BinlogWriter::onSpanAddedAttributeFloat32(SPECIAL_SPAN_ID_ROOT, string("kphp.script_time", 16), static_cast<float>(script_time));
    BinlogWriter::onSpanAddedAttributeInt32(SPECIAL_SPAN_ID_ROOT, string("workers.general", 15), std::get<3>(webserver_stats));
    BinlogWriter::onSpanAddedAttributeInt32(SPECIAL_SPAN_ID_ROOT, string("workers.general_free", 20), std::get<2>(webserver_stats));
    BinlogWriter::onSpanAddedAttributeInt32(SPECIAL_SPAN_ID_ROOT, string("workers.job", 11), f$get_job_workers_number());
    BinlogWriter::onSpanAddedAttributeInt32(SPECIAL_SPAN_ID_ROOT, string("kphp.binlog_size", 16), cur_binlog.calc_total_size() + 8);
    BinlogWriter::onSpanFinished(SPECIAL_SPAN_ID_ROOT, calc_time_offset(now_timestamp));

    // once per worker: flush enums (before any trace flush)
    if (!worker_process_has_sent_enums) {
      array<std::tuple<int64_t, string, array<string>>> enums_to_flush;
      if (cur_on_enums_callback) {
        enums_to_flush = cur_on_enums_callback();
      }
      enums_to_flush.push_back({SPECIAL_ENUM_TL_MAGICS, string("tl magics"), tl_magic_get_all_functions()});

      array<string> func_agg_enum;
      fill_BuiltinFuncID_strings(func_agg_enum);
      if (::kphp_tracing_autogen_func_call_strings) {
        ::kphp_tracing_autogen_func_call_strings(func_agg_enum);
      }
      enums_to_flush.push_back({SPECIAL_ENUM_FUNC_AGGREGATES, string("func aggregates"), func_agg_enum});

      for (const auto &it : enums_to_flush) {
        const auto &[enumID, enumName, enumKV] = it.get_value();
        snprintf(json_buf, 1024, R"({"trace_ctx":"","name":"flush_enum_%d:%s"})", static_cast<int>(enumID), enumName.c_str());
        tracing_binary_buffer flush_enum_binlog;
        flush_enum_binlog.append_enum_values(enumID, enumName, enumKV);
        flush_enum_binlog.output_to_json_log(json_buf);
      }
      worker_process_has_sent_enums = true;
    }

    // flush binlog strings inserted by previous requests with dropped traces
    // see kphp_tracing_binlog.cpp
    if (!flush_strings_binlog.empty()) {
      dl::CriticalSectionGuard lock;
      flush_strings_binlog.output_to_json_log(R"({"trace_ctx":"","name":"flush_strings"})");
      flush_strings_binlog.clear(true);
    }

    // flush current trace
    cur_trace->to_json(json_buf, 1024);
    cur_binlog.output_to_json_log(json_buf);

  } else if (cur_trace_level != -1) {
    // trace is dropped, but if it added new strings to binlog table, save that strings
    // see kphp_tracing_binlog.cpp
    dl::CriticalSectionGuard lock;
    flush_strings_binlog.append_current_php_script_strings();
  }
}

// when script finishes with an error (connection close, for example),
// we don't invoke php callbacks (we may be in net context)
// so, the trace is dropped, do the same as above
// todo think about OOM, we want to flush traces on soft OOM
void on_php_script_finish_terminated() {
  if (cur_trace_level != -1) {
    dl::CriticalSectionGuard lock;
    flush_strings_binlog.append_current_php_script_strings();
  }
}

} // namespace kphp_tracing


using kphp_tracing::BinlogWriter;
using kphp_tracing::calc_now_timestamp;
using kphp_tracing::calc_time_offset;
using kphp_tracing::cur_trace;


// class KphpDiv

// "Div" represents a piece of a distributed Trace written by a single process handling one request.
// It's a set of spans (KphpSpan below).
// In case of KPHP, when a worker starts handling a PHP request, it starts a div,
// which is finished on php script finish (no matter whether it's a general/job/rpc worker).
// A term "Div" was invented specially, not to be confused with a word "Trace":
// Trace is a set of Div with equal TraceID, it's a distributed entity,
// whereas Div is started and finished by a single trace-emitting node.


void C$KphpDiv::assign_random_trace_id_if_empty() {
  if (trace_id == 0) {
    trace_id = f$mt_rand(0, std::numeric_limits<int64_t>::max());
    if (kphp_tracing::is_turned_on()) {
      BinlogWriter::provideTraceContext(trace_id, trace_ctx_int2);
    }
  }
}

void C$KphpDiv::to_json(char *buf, size_t buf_len) const {
  auto str_to_json = [](const string &s) -> string {
    string escaped = s;
    escaped = f$str_replace(string("\\", 1), string("\\\\", 2), escaped);
    escaped = f$str_replace(string("\"", 1), string("\\\"", 2), escaped);
    return escaped;
  };

  snprintf(buf, buf_len, R"({"trace_ctx":"%016)" PRIx64 R"(%016)" PRIx64 R"(","name":"%s:%s","start":%.3f,"dur":%.3f})",
           trace_id, trace_ctx_int2,
           str_to_json(root_span_title).c_str(), str_to_json(root_span_desc).c_str(),
           start_timestamp, end_timestamp - start_timestamp);
}

double f$KphpDiv$$getStartTimestamp(const class_instance<C$KphpDiv> &v$this) {
  return v$this->start_timestamp;
}

double f$KphpDiv$$getEndTimestamp(const class_instance<C$KphpDiv> &v$this) {
  return v$this->end_timestamp;
}

std::tuple<int64_t, int64_t> f$KphpDiv$$generateTraceCtxForChild(const class_instance<C$KphpDiv> &v$this, int64_t div_id, int64_t trace_flags) {
  v$this->assign_random_trace_id_if_empty();
  return {v$this->trace_id, kphp_tracing::encode_trace_context_int2(div_id, trace_flags)};
}

int64_t f$KphpDiv$$assignTraceCtx(class_instance<C$KphpDiv> v$this, int64_t int1, int64_t int2, const Optional<int64_t> &override_div_id) {
  php_assert (v$this.get() == cur_trace.get());
  if (kphp_tracing::is_turned_on()) {
    v$this->trace_id = int1;
    v$this->trace_ctx_int2 = int2;
    if (override_div_id.has_value()) {
      if (override_div_id.val() < -(1_i64 << 31) || override_div_id.val() >= 1_i64 << 31) {
        php_warning("overflow of int32 in divID: %" PRIi64, override_div_id.val());
      }
      v$this->trace_ctx_int2 = (int2 & 0xFFFFFFFF) + (override_div_id << 32);
    }
    BinlogWriter::provideTraceContext(v$this->trace_id, v$this->trace_ctx_int2);
  }
  return v$this->trace_ctx_int2 & 0xFF;  // traceFlags
}


// class KphpSpan

// Span is an atomic element inside a Div.
// Spans in general have attributes, events, memory state, etc.,
// but KphpSpan stores no runtime state except span_id: instead, when spans are created/modified,
// these events are written into a binary buffer (binlog),
// so that a whole state can be replayed by a decoder.


void C$KphpSpan::addAttributeString(const string &key, const string &value) const {
  BinlogWriter::onSpanAddedAttributeString(span_id, key, value);
}

void C$KphpSpan::addAttributeInt(const string &key, int64_t value) const {
  if (value <= -(1_i64 << 31) || value >= 1_i64 << 31) {
    BinlogWriter::onSpanAddedAttributeInt64(span_id, key, value);
  } else {
    BinlogWriter::onSpanAddedAttributeInt32(span_id, key, value);
  }
}

void C$KphpSpan::addAttributeFloat(const string &key, double value) const {
  BinlogWriter::onSpanAddedAttributeFloat64(span_id, key, value);
}

void C$KphpSpan::addAttributeBool(const string &key, bool value) const {
  BinlogWriter::onSpanAddedAttributeBool(span_id, key, value);
}

void C$KphpSpan::addAttributeEnum(const string &key, int64_t enum_id, int64_t value) const {
  BinlogWriter::onSpanAddedAttributeInt32Enum(span_id, key, enum_id, value);
}

class_instance<C$KphpSpanEvent> C$KphpSpan::addEvent(const string &name, const Optional<double> &manual_timestamp) const {
  class_instance<C$KphpSpanEvent> ev;
  ev.alloc(++kphp_tracing::cur_last_span_id);
  double ev_timestamp = manual_timestamp.has_value() ? manual_timestamp.val() : calc_now_timestamp();
  BinlogWriter::onSpanAddedEvent(span_id, ev->span_id, name, calc_time_offset(ev_timestamp));
  return ev;
}

void C$KphpSpan::addLink(const class_instance<C$KphpSpan> &another) const {
  BinlogWriter::onSpanAddedLink(span_id, another->span_id);
}

void C$KphpSpan::updateName(const string &title, const string &short_desc) const {
  BinlogWriter::onSpanRenamed(span_id, title, short_desc);
  // rename root span => rename trace (just for human-readable json output, no other purpose)
  if (span_id == kphp_tracing::SPECIAL_SPAN_ID_ROOT) {
    cur_trace->root_span_title = title;
    cur_trace->root_span_desc = short_desc;
  }
}

void C$KphpSpan::finish(const Optional<double> &manual_timestamp) const {
  double end_timestamp = manual_timestamp.has_value() ? manual_timestamp.val() : calc_now_timestamp();
  kphp_tracing::provide_mem_info();
  BinlogWriter::onSpanFinished(span_id, calc_time_offset(end_timestamp));
}

void C$KphpSpan::finishWithError(int64_t error_code, const string &error_msg, const Optional<double> &manual_timestamp) const {
  double end_timestamp = manual_timestamp.has_value() ? manual_timestamp.val() : calc_now_timestamp();
  kphp_tracing::provide_mem_info();
  BinlogWriter::onSpanFinishedWithError(span_id, error_code, error_msg, calc_time_offset(end_timestamp));
}

void C$KphpSpan::exclude() const {
  BinlogWriter::onSpanExcluded(span_id);
}


// class KphpSpanEvent

// Returned by KphpSpan::addEvent(), can accept only attributes (which are also written to binlog).
// Actually, it's a special case of a span with kind=KindSpanEvent.


void C$KphpSpanEvent::addAttributeString(const string &key, const string &value) const {
  BinlogWriter::onSpanAddedAttributeString(span_id, key, value);
}

void C$KphpSpanEvent::addAttributeInt(const string &key, int64_t value) const {
  if (value <= -(1_i64 << 31) || value >= 1_i64 << 31) {
    BinlogWriter::onSpanAddedAttributeInt64(span_id, key, value);
  } else {
    BinlogWriter::onSpanAddedAttributeInt32(span_id, key, value);
  }
}

void C$KphpSpanEvent::addAttributeFloat(const string &key, double value) const {
  BinlogWriter::onSpanAddedAttributeFloat64(span_id, key, value);
}

void C$KphpSpanEvent::addAttributeBool(const string &key, bool value) const {
  BinlogWriter::onSpanAddedAttributeBool(span_id, key, value);
}


// --- global tracing functions

// They are declared in _functions.txt.


class_instance<C$KphpDiv> f$kphp_tracing_init(const string &root_span_title) {
  double now_timestamp = calc_now_timestamp();

  if (!cur_trace.is_null()) {
    php_warning("kphp_tracing_init() called more than once");
    return cur_trace;
  }

  kphp_tracing::cur_start_timestamp = now_timestamp;
  kphp_tracing::cur_trace_level = 1;
  kphp_tracing::cur_last_span_id = kphp_tracing::SPECIAL_SPAN_ID_ROOT;

  cur_trace.alloc();
  cur_trace->start_timestamp = now_timestamp;
  cur_trace->root_span_title = root_span_title;
  // trace_id is still unset, we even don't send it via binlog
  // either a PHP code manually sets it (provided by cookies / etc.)
  // or it will be generated randomly on demand or on finish

  kphp_tracing::cur_binlog.init_and_alloc();
  BinlogWriter::onSpanCreatedRoot(root_span_title, now_timestamp);

  return cur_trace;
}

void f$kphp_tracing_set_level(int64_t trace_level) {
  if (kphp_tracing::cur_trace_level == -1 || kphp_tracing::cur_trace_level == trace_level || trace_level < 0 || trace_level > 2) {
    return;
  }

  kphp_tracing::cur_trace_level = trace_level; // 0 may be used to disable tracing in the middle
  BinlogWriter::onSpanAddedAttributeInt32(kphp_tracing::SPECIAL_SPAN_ID_ROOT, string("trace_level", 11), trace_level);
}

int64_t f$kphp_tracing_get_level() {
  return kphp_tracing::cur_trace_level;
}

void f$kphp_tracing_register_on_finish(const kphp_tracing::on_trace_finish_callback_t &cb_should_be_flushed) {
  kphp_tracing::cur_on_finish_callback = cb_should_be_flushed;
}

void f$kphp_tracing_register_enums_provider(const kphp_tracing::on_trace_enums_callback_t &cb_custom_enums) {
  kphp_tracing::cur_on_enums_callback = cb_custom_enums;
}

void f$kphp_tracing_register_rpc_details_provider(const kphp_tracing::on_rpc_provide_details_typed_t &cb_for_typed, const kphp_tracing::on_rpc_provide_details_untyped_t &cb_for_untyped) {
  kphp_tracing::cur_on_rpc_details_typed_callback = cb_for_typed;
  kphp_tracing::cur_on_rpc_details_untyped_callback = cb_for_untyped;
}

class_instance<C$KphpSpan> f$kphp_tracing_start_span(const string &title, const string &short_desc, double start_timestamp) {
  class_instance<C$KphpSpan> span;
  span.alloc(++kphp_tracing::cur_last_span_id);
  if (kphp_tracing::cur_trace_level >= 1) {
    kphp_tracing::provide_mem_info();
    if (short_desc.empty()) {
      BinlogWriter::onSpanCreatedTitleOnly(span->span_id, title, calc_time_offset(start_timestamp));
    } else {
      BinlogWriter::onSpanCreatedTitleDesc(span->span_id, title, short_desc, calc_time_offset(start_timestamp));
    }
  }
  return span;  // always return a valid object, even if tracing is disabled
}

class_instance<C$KphpSpan> f$kphp_tracing_get_root_span() {
  class_instance<C$KphpSpan> span;
  span.alloc(kphp_tracing::SPECIAL_SPAN_ID_ROOT);
  return span;
}

class_instance<C$KphpSpan> f$kphp_tracing_get_current_active_span() {
  class_instance<C$KphpSpan> span;
  span.alloc(kphp_tracing::SPECIAL_SPAN_ID_CURRENT_ACTIVE);
  return span;
}


// function calls: `@kphp-tracing`

// When a php function is marked with `@kphp-tracing`, the compiler inserts
// `KphpTracingFuncCallGuard _tr_f; _tr_f.start(...)` at code generation.
// It starts a new span in `start()` and finishes it in destructor (that's why all returns are covered).
// For resumable functions, _tr_f is also a field in a class extending Resumable,
// and start() is code generated just after RESUMABLE_BEGIN
// (that's the reason start() is needed to be called separately from a constructor, after fork creation).
// For `@kphp-tracing level=2`, trace_level is just passed 2 into start().
//
// Note, that `@kphp-tracing aggregate` generates a separate guard:
// `KphpTracingAggregateGuard _tr_f; _tr_f.start(...)` at code generation.
// Resumable functions are also supported in the same way as for KphpTracingFuncCallGuard.
// The main difference that "aggregate" is placed for functions that are called very often:
// in UI, we don't want to see every call as a separate span:
// instead, we want to see a number of calls and time spent in total.
// They are turned only for advanced trace level. For optimization, a enum of all such functions/aggregates
// is also code generated by the compiler, and we need to pass only 24-bit call masks at start/finish.


static_assert(sizeof(KphpTracingFuncCallGuard) == 4);
static_assert(sizeof(KphpTracingAggregateGuard) == 4);

void KphpTracingFuncCallGuard::on_started(const char *f_name, int len) {
  double now_timestamp = calc_now_timestamp();
  span_id = ++kphp_tracing::cur_last_span_id;
  kphp_tracing::provide_mem_info();
  string span_name(f_name, len);
  BinlogWriter::onFuncCallStarted(span_id, span_name, calc_time_offset(now_timestamp));
}

void KphpTracingFuncCallGuard::on_finished() const {
  double now_timestamp = calc_now_timestamp();
  kphp_tracing::provide_mem_info();
  BinlogWriter::onFuncCallFinished(span_id, calc_time_offset(now_timestamp));
}

void KphpTracingAggregateGuard::on_started(int call_mask) {
  func_call_mask = call_mask;
  double now_timestamp = calc_now_timestamp();
  BinlogWriter::onFuncAggregateStarted(func_call_mask, calc_time_offset(now_timestamp));
}

void KphpTracingAggregateGuard::on_enter_branch(int branch_num) const {
  int mask = (func_call_mask & 0xFFFF) | (branch_num << static_cast<int>(kphp_tracing::BuiltinFuncID::_shift_for_branch));
  BinlogWriter::onFuncAggregateBranch(mask);
}

void KphpTracingAggregateGuard::on_finished() const {
  double now_timestamp = calc_now_timestamp();
  BinlogWriter::onFuncAggregateFinished(func_call_mask, calc_time_offset(now_timestamp));
}
