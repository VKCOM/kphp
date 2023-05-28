// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/calc-const-types.h"

#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/data/var-data.h"

void CalcConstTypePass::on_start() {
  if (current_function->type == FunctionData::func_class_holder) {
    calc_const_type_of_class_fields(current_function->class_id);
  }
}

void CalcConstTypePass::calc_const_type_of_class_fields(ClassPtr klass) {
  klass->members.for_each([this](ClassMemberStaticField &f) {
    if (f.var->init_val) {
      run_function_pass(f.var->init_val, this);
      kphp_error(f.var->init_val->const_type == cnst_const_val, fmt_format("Default value of {} is not constant", f.var->as_human_readable()));
    }
  });
  klass->members.for_each([this](ClassMemberInstanceField &f) {
    if (f.var->init_val) {
      run_function_pass(f.var->init_val, this);
      kphp_error(f.var->init_val->const_type == cnst_const_val, fmt_format("Default value of {} is not constant", f.var->as_human_readable()));
    }
  });
}

VertexPtr CalcConstTypePass::on_exit_vertex(VertexPtr v) {
  if (auto as_define_val = v.try_as<op_define_val>()) {
    inlined_define_cnt--;
    auto val = as_define_val->value();
    // check if it's an object in define
    as_define_val->const_type = val->const_type;
    kphp_error(as_define_val->const_type == cnst_const_val, "Non-const expression in const context");
    return v;
  }
  // for now only pure functions and
  // constructors in defines can be `cnst_const_val`-ed
  if (auto as_func_call = v.try_as<op_func_call>()) {
    const bool can_be_const = as_func_call->func_id && ((as_func_call->func_id->is_constructor() && inlined_define_cnt > 0)|| as_func_call->func_id->is_pure);
    if (!can_be_const) {
      v->const_type = cnst_nonconst_val;
      return v;
    }
  }
  switch (OpInfo::cnst(v->type())) {
    case cnst_func:
    case cnst_const_func: {
      bool has_nonconst_son = vk::any_of(*v, [](VertexPtr son) { return son->const_type == cnst_nonconst_val; });
      v->const_type = has_nonconst_son ? cnst_nonconst_val : cnst_const_val;
      break;
    }
    case cnst_nonconst_func: {
      v->const_type = cnst_nonconst_val;
      break;
    }
    case cnst_not_func: {
      v->const_type = cnst_not_val;
      break;
    }
    default:
      kphp_error (0, fmt_format("Unknown cnst-type for [op = {}]", v->type()));
      kphp_fail();
      break;
  }
  return v;
}

VertexPtr CalcConstTypePass::on_enter_vertex(VertexPtr v) {
  if (v->type() == op_define_val) {
    inlined_define_cnt++;
  }
  return v;
}
