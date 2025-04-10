// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "common/wrappers/likely.h"
#include "runtime-common/stdlib/tracing/tracing-context.h"

namespace kphp_tracing {

// see comments about `@kphp-tracing aggregate`
enum class BuiltinFuncID : int {
  _max_user_defined_functions = 64000, // and 1.5k for built-in functions (16 bits total)
  _max_user_defined_aggregate = 24,    // and 7 for built-in aggregates (5 bits total)
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

} // namespace kphp_tracing

// function calls: autogen from `@kphp-tracing aggregate`

class KphpTracingAggregateGuard {
  int32_t func_call_mask{};

  void on_started(int32_t func_call_mask) noexcept;
  void on_enter_branch(int32_t branch_num) const noexcept;
  void on_finished() const noexcept;

public:
  KphpTracingAggregateGuard() noexcept = default;
  KphpTracingAggregateGuard(const KphpTracingAggregateGuard&) = delete;
  KphpTracingAggregateGuard& operator=(const KphpTracingAggregateGuard&) = delete;

  void start(int32_t call_mask) {
    if (unlikely(kphp_tracing::TracingContext::get().cur_trace_level >= 2)) {
      on_started(call_mask);
    }
  }
  explicit KphpTracingAggregateGuard(kphp_tracing::BuiltinFuncID func_id, kphp_tracing::BuiltinFuncID aggregate_bits) {
    if (unlikely(kphp_tracing::TracingContext::get().cur_trace_level >= 2)) {
      on_started(static_cast<int>(func_id) | static_cast<int>(aggregate_bits));
    }
  }

  void enter_branch(int32_t branch_num) {
    // codegenerated instead of kphp_tracing_func_enter_branch(N)
    if (unlikely(kphp_tracing::TracingContext::get().cur_trace_level >= 2)) {
      on_enter_branch(branch_num);
    }
  }
  void enter_branch(kphp_tracing::BuiltinFuncID branch_num) {
    if (unlikely(kphp_tracing::TracingContext::get().cur_trace_level >= 2)) {
      on_enter_branch(static_cast<int>(branch_num));
    }
  }

  ~KphpTracingAggregateGuard() {
    if (unlikely(kphp_tracing::TracingContext::get().cur_trace_level >= 2)) {
      on_finished();
    }
  }
};
