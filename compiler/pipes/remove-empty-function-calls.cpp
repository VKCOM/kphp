// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/remove-empty-function-calls.h"

#include "compiler/vertex.h"
#include "compiler/data/src-file.h"
#include "compiler/compiler-core.h"

static bool is_empty_func_call(VertexPtr v) {
  return v->type() == op_func_call && v.as<op_func_call>()->func_id->body_seq == FunctionData::body_value::empty;
};

VertexPtr RemoveEmptyFunctionCallsPass::on_enter_vertex(VertexPtr v) {
  if (auto return_v = v.try_as<op_return>()) {
    if (return_v->has_expr() && is_empty_func_call(return_v->expr())) {
      return VertexAdaptor<op_return>::create().set_location(return_v);
    }
  }
  return v;
}

VertexPtr RemoveEmptyFunctionCallsPass::on_exit_vertex(VertexPtr v) {
  if (is_empty_func_call(v)) {
    return VertexAdaptor<op_null>::create();
  }

  if (v->type() == op_seq) {
    auto is_empty = [](VertexPtr v) { return v->type() == op_null || v->type() == op_empty;};
    if (std::any_of(v->begin(), v->end(), is_empty)) {
      std::vector<VertexPtr> next;
      std::remove_copy_if(v->begin(), v->end(), std::back_inserter(next), is_empty);
      return VertexAdaptor<op_seq>::create(next);
    }
  }
  if (auto if_v = v.try_as<op_if>()) {
    auto cond = if_v->cond();
    if (cond->type() == op_log_not && cond.as<op_log_not>()->expr()->type() == op_var && !if_v->has_false_cmd()) {
      if (if_v->true_cmd()->type() == op_seq && if_v->true_cmd()->size() == 0) {
        return VertexAdaptor<op_empty>::create();
      }
    }
  }
  if (v->type() == op_set) {
    // get rid of $called - global variables for empty source files;
    // namely, detect 'v$src_fooxxx$called = true' assign in such files and remove it,
    // this allows to avoid further call of register_var() with such global variable
    if (!G->is_output_mode_lib() && !G->is_output_mode_k2_lib() && current_function->is_main_function() && current_function->body_seq == FunctionData::body_value::empty) {
      auto set = v.as<op_set>();
      auto lhs = set->lhs();
      auto rhs = set->rhs();
      if (rhs->type() == op_true && lhs->type() == op_var && lhs->get_string() == current_function->file_id->get_main_func_run_var_name()) {
        return VertexAdaptor<op_empty>::create();
      }
    }
  }
  return v;
}
