// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/calc-empty-functions.h"

#include "compiler/data/function-data.h"
#include "compiler/data/src-file.h"
#include "compiler/function-pass.h"
#include "compiler/vertex.h"

namespace {

FunctionData::body_value calc_function_body_type(FunctionPtr f);
FunctionData::body_value calc_seq_body_type(VertexAdaptor<op_seq> seq);

FunctionData::body_value get_vertex_body_type(VertexPtr vertex) {
  switch (vertex->type()) {
    case op_set: {
      FunctionPtr fun = stage::get_function();
      if (fun->file_id->main_function == fun) {
        auto set = vertex.as<op_set>();
        auto lhs = set->lhs();
        auto rhs = set->rhs();
        if (rhs->type() == op_true && lhs->type() == op_var && lhs->get_string() == fun->file_id->get_main_func_run_var_name()) {
          return FunctionData::body_value::empty;
        }
      }
      return FunctionData::body_value::non_empty;
    }
    case op_if: {
      auto if_v = vertex.as<op_if>();
      auto cond = if_v->cond();
      if (cond->type() == op_log_not && cond.as<op_log_not>()->expr()->type() == op_var && !if_v->has_false_cmd()) {
        return get_vertex_body_type(if_v->true_cmd());
      }
      return FunctionData::body_value::non_empty;
    }
    case op_func_call:
      return FunctionData::body_value::unknown;
    case op_return: {
      auto return_vertex = vertex.as<op_return>();
      if (!return_vertex->has_expr() || (return_vertex->expr()->type() == op_null && stage::get_function()->is_main_function())) {
        return FunctionData::body_value::empty;
      } else {
        return get_vertex_body_type(return_vertex->expr());
      }
    }
    case op_seq:
      return calc_seq_body_type(vertex.as<op_seq>());
    case op_empty:
      return FunctionData::body_value::empty;
    default:
      return FunctionData::body_value::non_empty;
  }
}

FunctionData::body_value calc_seq_body_type(VertexAdaptor<op_seq> seq) {
  FunctionData::body_value result = FunctionData::body_value::empty;
  for (VertexPtr seq_element : *seq) {
    FunctionData::body_value vertex_body = get_vertex_body_type(seq_element);
    if (vertex_body == FunctionData::body_value::non_empty) {
      return FunctionData::body_value::non_empty;
    }
    if (vertex_body == FunctionData::body_value::unknown) {
      result = FunctionData::body_value::unknown;
    }
  }
  return result;
}

FunctionData::body_value calc_function_body_type(FunctionPtr f) {
  kphp_assert(f->body_seq == FunctionData::body_value::unknown);

  if (f->is_extern() || f->type == FunctionData::func_switch || f->type == FunctionData::func_class_holder || f->root->type() != op_function
      || !f->get_params().empty()) {
    return FunctionData::body_value::non_empty;
  }

  return calc_seq_body_type(f->root->cmd());
}

} // namespace

void CalcEmptyFunctions::execute(FunctionPtr f, DataStream<FunctionPtr> &os) {
  stage::set_function(f);
  f->body_seq = calc_function_body_type(f);
  os << f;
}
