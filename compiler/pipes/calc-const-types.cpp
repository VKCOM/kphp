#include "compiler/pipes/calc-const-types.h"

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
