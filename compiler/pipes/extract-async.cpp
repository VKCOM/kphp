// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/extract-async.h"

#include <atomic>

bool ExtractAsyncPass::check_function(FunctionPtr function) const {
  return !function->is_extern() && function->is_resumable;
}

VertexPtr ExtractAsyncPass::on_exit_vertex(VertexPtr vertex) {
  if (vertex->rl_type != val_none) {
    return vertex;
  }
  auto get_resumable_func_call = [](VertexPtr v) {
    auto func_call = v.try_as<op_func_call>();
    if (func_call && func_call->func_id->is_resumable) {
      return func_call;
    }
    return VertexAdaptor<op_func_call>{};
  };

  if (auto func_call = get_resumable_func_call(vertex)) {
    return VertexAdaptor<op_async>::create(func_call).set_location(func_call);
  }
  if (auto set = vertex.try_as<op_set>()) {
    if (auto func_call = get_resumable_func_call(set->rhs())) {
      if (auto lhs_var = set->lhs().try_as<op_var>()) {
        return VertexAdaptor<op_async>::create(func_call, lhs_var).set_location(func_call);
      } else {
        raise_cant_save_result_of_resumable_func(func_call->func_id);
      }
    }
  }
  return vertex;
}

void ExtractAsyncPass::raise_cant_save_result_of_resumable_func(FunctionPtr func) {
  static std::atomic<uint32_t> errors_count;
  static constexpr uint32_t max_errors_count = 10;
  auto cnt = ++errors_count;
  if (cnt < max_errors_count) {
    kphp_error(false, fmt_format("Can't save result of resumable function {} into non-var inside another resumable function\n"
                                 "Consider using a temporary variable for this call.\n"
                                 "Function is resumable because of calls chain:\n"
                                 "{}",
                                 func->as_human_readable(), func->get_resumable_path()));
  } else if (cnt == max_errors_count) {
    kphp_error(false, "Too many same errors about resumable functions, will skip others");
  }
}
