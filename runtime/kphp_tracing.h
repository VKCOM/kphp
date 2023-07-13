// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <functional>

#include "runtime/kphp_core.h"
#include "runtime/refcountable_php_classes.h"

// for detailed comments about tracing in general, see kphp_tracing.cpp

struct C$KphpDiv;
struct C$KphpSpan;
struct C$KphpSpanEvent;
struct C$VK$TL$RpcFunction;

namespace kphp_tracing {

// see comments about `@kphp-tracing aggregate`
enum class BuiltinFuncID : int {
  _max_user_defined_functions = 64000,  // and 1.5k for built-in functions (16 bits total)
  _max_user_defined_aggregate = 24,     // and 7 for built-in aggregates (5 bits total)
  _shift_for_branch = 16,
  _shift_for_aggregate = 18,
  _shift_for_reserved = 23,

  preg_match = _max_user_defined_functions + 1,
  preg_match_all,
  preg_split,
  preg_replace,
  preg_replace_callback,
  exec,
  system,
  passthru,

  branch_regex_needs_compilation = 1,

  aggregate_regexp_functions = (_max_user_defined_aggregate + 1) << _shift_for_aggregate,
};

// cur_trace_level is level of tracing for current php script
// -1 - not inited, disabled (on php script finish, it's reset to -1)
// 0  - inited, but turned off
//      (binlog is not written, on_rpc_query_send() and other "callbacks" are not called)
// 1  - turned on, this php script is traced in a regular mode
//      (binlog is written, and when the script finishes, it's either flushed to a file or dismissed)
// 2  - turned on in advanced mode
//      (same as 1, but more events with more details are written to binlog, significant overhead)
extern int cur_trace_level;

[[gnu::always_inline]] inline bool is_turned_on() {
  return cur_trace_level >= 1;
}

int generate_uniq_id();

using on_trace_finish_callback_t = std::function<bool(double)>;
using on_trace_enums_callback_t = std::function<array<std::tuple<int64_t, string, array<string>>>()>;
using on_rpc_provide_details_typed_t = std::function<string(class_instance<C$VK$TL$RpcFunction>)>;
using on_rpc_provide_details_untyped_t = std::function<string(mixed)>;

void init_tracing_lib();
void free_tracing_lib();

// these functions are "injections" into KPHP runtime, they are directly called from various places

void on_fork_start(int64_t fork_id);
void on_fork_finish(int64_t fork_id);
void on_fork_switch(int64_t fork_id);
void on_fork_provide_name(int64_t fork_id, const string &fork_typeid);

void on_rpc_query_send(int rpc_query_id, int actor_or_port, unsigned int tl_magic, int bytes_sent, double start_timestamp, bool is_no_result);
void on_rpc_query_finish(int rpc_query_id, int bytes_recv);
void on_rpc_query_fail(int rpc_query_id, int error_code);
void on_rpc_query_provide_details_after_send(const class_instance<C$VK$TL$RpcFunction> &typed_f, const mixed &untyped_obj);

void on_job_worker_start(int job_id, const string &job_class_name, double start_timestamp, bool is_no_reply);
void on_job_worker_finish(int job_id);
void on_job_worker_fail(int job_id, int error_code);

void on_curl_exec_start(int curl_handle, const string &url);
void on_curl_exec_finish(int curl_handle, int bytes_recv);
void on_curl_exec_fail(int curl_handle, int error_code);
void on_curl_add_attribute(int curl_handle, const string &key, const string &value);
void on_curl_add_attribute(int curl_handle, const string &key, int value);

void on_curl_multi_init(int multi_handle);
void on_curl_multi_add_handle(int multi_handle, int curl_handle, const string &url);
void on_curl_multi_remove_handle(int multi_handle, int curl_handle, int bytes_recv);
void on_curl_multi_close(int multi_handle);

void on_external_program_start(int exec_id, BuiltinFuncID func_id, const string &command);
void on_external_program_finish(int exec_id, bool success, int64_t exit_code);

void on_file_io_start(int fd, const string &file_name, bool is_write);
void on_file_io_finish(int fd, int bytes);
void on_file_io_fail(int fd, int error_code);

void on_switch_context_to_script(double net_time_delta);
void on_shutdown_functions_start(int shutdown_type_enum);
void on_php_script_warning(const string &warning_message);
void on_php_script_finish_ok(double net_time, double script_time);
void on_php_script_finish_terminated();

} // namespace kphp_tracing

// class KphpDiv

struct C$KphpDiv : public refcountable_php_classes<C$KphpDiv> {
  double start_timestamp{0.0};  // all fields are inaccessible from PHP code
  double end_timestamp{0.0};    // (PHP code can only call f$KphpDiv$$ functions)
  int64_t trace_id{0};
  int64_t trace_ctx_int2{0};
  string root_span_title;
  string root_span_desc;

  void assign_random_trace_id_if_empty();
  void to_json(char *buf, size_t buf_len) const;
};

double f$KphpDiv$$getStartTimestamp(const class_instance<C$KphpDiv> &v$this);
double f$KphpDiv$$getEndTimestamp(const class_instance<C$KphpDiv> &v$this);
std::tuple<int64_t, int64_t> f$KphpDiv$$generateTraceCtxForChild(const class_instance<C$KphpDiv> &v$this, int64_t div_id, int64_t trace_flags);
int64_t f$KphpDiv$$assignTraceCtx(class_instance<C$KphpDiv> v$this, int64_t int1, int64_t int2, const Optional<int64_t> &override_div_id);


// class KphpSpan

struct C$KphpSpan : public refcountable_php_classes<C$KphpSpan> {
  int span_id{0};

  C$KphpSpan() = default;
  explicit C$KphpSpan(int span_id) : span_id(span_id) {}

  void addAttributeString(const string &key, const string &value) const;
  void addAttributeInt(const string &key, int64_t value) const;
  void addAttributeFloat(const string &key, double value) const;
  void addAttributeBool(const string &key, bool value) const;
  void addAttributeEnum(const string &key, int64_t enum_id, int64_t value) const;

  class_instance<C$KphpSpanEvent> addEvent(const string &name, const Optional<double> &manual_timestamp) const;
  void addLink(const class_instance<C$KphpSpan> &another) const;

  void updateName(const string &title, const string &short_desc) const;
  void finish(const Optional<double> &manual_timestamp) const;
  void finishWithError(int64_t error_code, const string &error_msg, const Optional<double> &manual_timestamp) const;
  void exclude() const;
};

inline void f$KphpSpan$$addAttributeString(const class_instance<C$KphpSpan> &v$this, const string &key, const string &value) {
  if (kphp_tracing::cur_trace_level >= 1) {
    v$this->addAttributeString(key, value);
  }
}
inline void f$KphpSpan$$addAttributeInt(const class_instance<C$KphpSpan> &v$this, const string &key, int64_t value) {
  if (kphp_tracing::cur_trace_level >= 1) {
    v$this->addAttributeInt(key, value);
  }
}
inline void f$KphpSpan$$addAttributeFloat(const class_instance<C$KphpSpan> &v$this, const string &key, double value) {
  if (kphp_tracing::cur_trace_level >= 1) {
    v$this->addAttributeFloat(key, value);
  }
}
inline void f$KphpSpan$$addAttributeBool(const class_instance<C$KphpSpan> &v$this, const string &key, bool value) {
  if (kphp_tracing::cur_trace_level >= 1) {
    v$this->addAttributeBool(key, value);
  }
}
inline void f$KphpSpan$$addAttributeEnum(const class_instance<C$KphpSpan> &v$this, const string &key, int64_t enum_id, int64_t value) {
  if (kphp_tracing::cur_trace_level >= 1) {
    v$this->addAttributeEnum(key, enum_id, value);
  }
}
inline class_instance<C$KphpSpanEvent> f$KphpSpan$$addEvent(const class_instance<C$KphpSpan> &v$this, const string &name, const Optional<double> &manual_timestamp = {}) {
  if (kphp_tracing::cur_trace_level >= 1) {
    return v$this->addEvent(name, manual_timestamp);
  }
  return {};
}
inline void f$KphpSpan$$addLink(const class_instance<C$KphpSpan> &v$this, const class_instance<C$KphpSpan> &another) {
  if (kphp_tracing::cur_trace_level >= 1) {
    v$this->addLink(another);
  }
}
inline void f$KphpSpan$$updateName(const class_instance<C$KphpSpan> &v$this, const string &title, const string &short_desc) {
  if (kphp_tracing::cur_trace_level >= 1) {
    v$this->updateName(title, short_desc);
  }
}
inline void f$KphpSpan$$finish(const class_instance<C$KphpSpan> &v$this, const Optional<double> &manual_timestamp = {}) {
  if (kphp_tracing::cur_trace_level >= 1) {
    v$this->finish(manual_timestamp);
  }
}
inline void f$KphpSpan$$finishWithError(const class_instance<C$KphpSpan> &v$this, int64_t error_code, const string &error_msg, const Optional<float> &manual_timestamp = {}) {
  if (kphp_tracing::cur_trace_level >= 1) {
    v$this->finishWithError(error_code, error_msg, manual_timestamp);
  }
}
inline void f$KphpSpan$$exclude(const class_instance<C$KphpSpan> &v$this) {
  if (kphp_tracing::cur_trace_level >= 1) {
    v$this->exclude();
  }
}


// class KphpSpanEvent

struct C$KphpSpanEvent : public refcountable_php_classes<C$KphpSpanEvent> {
  int span_id{0};

  C$KphpSpanEvent() = default;
  explicit C$KphpSpanEvent(int span_id) : span_id(span_id) {}

  void addAttributeString(const string &key, const string &value) const;
  void addAttributeInt(const string &key, int64_t value) const;
  void addAttributeFloat(const string &key, double value) const;
  void addAttributeBool(const string &key, bool value) const;
};

inline void f$KphpSpanEvent$$addAttributeString(const class_instance<C$KphpSpanEvent> &v$this, const string &key, const string &value) {
  if (kphp_tracing::cur_trace_level >= 1) {
    v$this->addAttributeString(key, value);
  }
}
inline void f$KphpSpanEvent$$addAttributeInt(const class_instance<C$KphpSpanEvent> &v$this, const string &key, int64_t value) {
  if (kphp_tracing::cur_trace_level >= 1) {
    v$this->addAttributeInt(key, value);
  }
}
inline void f$KphpSpanEvent$$addAttributeFloat(const class_instance<C$KphpSpanEvent> &v$this, const string &key, double value) {
  if (kphp_tracing::cur_trace_level >= 1) {
    v$this->addAttributeFloat(key, value);
  }
}
inline void f$KphpSpanEvent$$addAttributeBool(const class_instance<C$KphpSpanEvent> &v$this, const string &key, bool value) {
  if (kphp_tracing::cur_trace_level >= 1) {
    v$this->addAttributeBool(key, value);
  }
}


// global tracing functions

class_instance<C$KphpDiv> f$kphp_tracing_init(const string &root_span_title);
void f$kphp_tracing_set_level(int64_t trace_level);
int64_t f$kphp_tracing_get_level();

void f$kphp_tracing_register_on_finish(const kphp_tracing::on_trace_finish_callback_t &cb_should_be_flushed);
void f$kphp_tracing_register_enums_provider(const kphp_tracing::on_trace_enums_callback_t &cb_custom_enums);
void f$kphp_tracing_register_rpc_details_provider(const kphp_tracing::on_rpc_provide_details_typed_t &cb_for_typed, const kphp_tracing::on_rpc_provide_details_untyped_t &cb_for_untyped);

class_instance<C$KphpSpan> f$kphp_tracing_start_span(const string &title, const string &short_desc, double start_timestamp);
class_instance<C$KphpSpan> f$kphp_tracing_get_root_span();
class_instance<C$KphpSpan> f$kphp_tracing_get_current_active_span();


// function calls: autogen from `@kphp-tracing`

class KphpTracingFuncCallGuard {
  int span_id{0};

  void on_started(const char *f_name, int len);
  void on_finished() const;

public:
  KphpTracingFuncCallGuard(const KphpTracingFuncCallGuard &) = delete;
  KphpTracingFuncCallGuard &operator=(const KphpTracingFuncCallGuard &) = delete;

  KphpTracingFuncCallGuard() {
  }
  inline void start(const char *f_name, int len, int trace_level) {
    if (kphp_tracing::cur_trace_level >= trace_level) {   // 1 or 2
      on_started(f_name, len);
    }
  }
  
  ~KphpTracingFuncCallGuard() {
    if (kphp_tracing::cur_trace_level >= 1 && span_id) {
      on_finished();
    }
  }
};

// function calls: autogen from `@kphp-tracing aggregate`

class KphpTracingAggregateGuard {
  int func_call_mask;

  void on_started(int func_call_mask);
  void on_enter_branch(int branch_num) const;
  void on_finished() const;

public:
  KphpTracingAggregateGuard(const KphpTracingAggregateGuard &) = delete;
  KphpTracingAggregateGuard &operator=(const KphpTracingAggregateGuard &) = delete;

  KphpTracingAggregateGuard() {
    // again, constructor is empty, does not even assign 0 to func_call_mask,
    // because start() is codegenerated either immediately of in RESUMABLE_BEGIN
  }
  void start(int call_mask) {
    if (unlikely(kphp_tracing::cur_trace_level >= 2)) {
      on_started(call_mask);
    }
  }
  explicit KphpTracingAggregateGuard(kphp_tracing::BuiltinFuncID func_id, kphp_tracing::BuiltinFuncID aggregate_bits) {
    if (unlikely(kphp_tracing::cur_trace_level >= 2)) {
      on_started(static_cast<int>(func_id) | static_cast<int>(aggregate_bits));
    }
  }
  
  void enter_branch(int branch_num) {
    // codegenerated instead of kphp_tracing_func_enter_branch(N)
    if (unlikely(kphp_tracing::cur_trace_level >= 2)) {
      on_enter_branch(branch_num);
    }
  }
  void enter_branch(kphp_tracing::BuiltinFuncID branch_num) {
    if (unlikely(kphp_tracing::cur_trace_level >= 2)) {
      on_enter_branch(static_cast<int>(branch_num));
    }
  }

  ~KphpTracingAggregateGuard() {
    if (unlikely(kphp_tracing::cur_trace_level >= 2)) {
      on_finished();
    }
  }
};

