#include "compiler/pipes/calc-const-types.h"

#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/data/var-data.h"

bool CalcConstTypePass::on_start(FunctionPtr function) {
  if (!FunctionPassBase::on_start(function)) {
    return false;
  }

  if (current_function->type == FunctionData::func_class_holder) {
    calc_const_type_of_class_fields(current_function->class_id);
  }
  return true;
}

void CalcConstTypePass::calc_const_type_of_class_fields(ClassPtr klass) {
  klass->members.for_each([&](ClassMemberStaticField &f) {
    if (f.var->init_val) {
      run_function_pass(f.var->init_val, this);
      kphp_error(f.var->init_val->const_type == cnst_const_val, fmt_format("Default value of {}::${} is not constant", klass->name, f.local_name()));
    }
  });
  klass->members.for_each([&](ClassMemberInstanceField &f) {
    if (f.var->init_val) {
      run_function_pass(f.var->init_val, this);
      kphp_error(f.var->init_val->const_type == cnst_const_val, fmt_format("Default value of {}::${} is not constant", klass->name, f.local_name()));
    }
  });
}

VertexPtr CalcConstTypePass::on_exit_vertex(VertexPtr v) {
  bool has_nonconst = false;
  for (VertexPtr son : *v) {
    has_nonconst |= son->const_type == cnst_nonconst_val;
  }
  switch (OpInfo::cnst(v->type())) {
    case cnst_func:
      if (auto as_func_call = v.try_as<op_func_call>()) {
        auto root = as_func_call->func_id ? as_func_call->func_id->root : VertexAdaptor<op_function>{};
        if (!root || !root->type_rule || root->type_rule->rule()->extra_type != op_ex_rule_const) {
          v->const_type = cnst_nonconst_val;
          break;
        }
      }
      /* fallthrough */
    case cnst_const_func:
      v->const_type = has_nonconst ? cnst_nonconst_val : cnst_const_val;
      break;
    case cnst_nonconst_func:
      v->const_type = cnst_nonconst_val;
      break;
    case cnst_not_func:
      v->const_type = cnst_not_val;
      break;
    default:
      kphp_error (0, fmt_format("Unknown cnst-type for [op = {}]", v->type()));
      kphp_fail();
      break;
  }
  return v;
}
