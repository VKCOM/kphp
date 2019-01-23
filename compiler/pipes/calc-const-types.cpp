#include "compiler/pipes/calc-const-types.h"

#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"

bool CalcConstTypePass::on_start(FunctionPtr function) {
  if (!FunctionPassBase::on_start(function)) {
    return false;
  }

  if (current_function->type == FunctionData::func_class_holder) {
    current_function->class_id->members.for_each([&](ClassMemberStaticField &f) {
      LocalT local;
      f.init_val = run_function_pass(f.init_val, this, &local);
    });
  }
  return true;
}

void CalcConstTypePass::on_exit_edge(VertexPtr, LocalT *v_local, VertexPtr from, LocalT *) {
  v_local->has_nonconst |= from->const_type == cnst_nonconst_val;
}

VertexPtr CalcConstTypePass::on_exit_vertex(VertexPtr v, LocalT *local) {
  switch (OpInfo::cnst(v->type())) {
    case cnst_func:
      if (v->get_func_id()) {
        VertexPtr root = v->get_func_id()->root;
        if (!root || !root->type_rule || root->type_rule->extra_type != op_ex_rule_const) {
          v->const_type = cnst_nonconst_val;
          break;
        }
      }
      /* fallthrough */
    case cnst_const_func:
      v->const_type = local->has_nonconst ? cnst_nonconst_val : cnst_const_val;
      break;
    case cnst_nonconst_func:
      v->const_type = cnst_nonconst_val;
      break;
    case cnst_not_func:
      v->const_type = cnst_not_val;
      break;
    default:
      kphp_error (0, format("Unknown cnst-type for [op = %d]", v->type()));
      kphp_fail();
      break;
  }
  return v;
}
