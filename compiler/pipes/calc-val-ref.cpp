#include "compiler/pipes/calc-val-ref.h"

#include "compiler/inferring/public.h"

bool CalcValRefPass::is_allowed_for_getting_val_or_ref(Operation op, bool is_last) {
  switch (op) {
    case op_push_back:
    case op_push_back_return:
    case op_set_value:
      return !is_last;

    case op_list:
      return is_last;

    case op_minus:
    case op_plus:

    case op_set_add:
    case op_set_sub:
    case op_set_mul:
    case op_set_div:
    case op_set_mod:
    case op_set_pow:
    case op_set_and:
    case op_set_or:
    case op_set_xor:
    case op_set_shr:
    case op_set_shl:

    case op_add:
    case op_sub:
    case op_mul:
    case op_div:
    case op_mod:
    case op_pow:
    case op_not:
    case op_log_not:

    case op_prefix_inc:
    case op_prefix_dec:
    case op_postfix_inc:
    case op_postfix_dec:

    case op_exit:
    case op_conv_int:
    case op_conv_int_l:
    case op_conv_float:
    case op_conv_array: // ?
    case op_conv_array_l: // ?
    case op_conv_string_l:
    case op_conv_uint:
    case op_conv_long:
    case op_conv_ulong:
      // case op_conv_bool нет намеренно, чтобы f$boolval(OrFalse<T>) не превращался в дефолтный T

    case op_isset:
    case op_unset:
    case op_index:
    case op_switch:
      return true;

    default:
      return false;
  }
}
void CalcValRefPass::on_enter_edge(VertexPtr, LocalT *local, VertexPtr dest_vertex, LocalT *) {
  if (local->allowed && dest_vertex->rl_type != val_none && dest_vertex->rl_type != val_error) {
    const TypeData *tp = tinf::get_type(dest_vertex);

    if (tp->ptype() != tp_Unknown && tp->use_or_false()) {
      dest_vertex->val_ref_flag = dest_vertex->rl_type;
    }
  }
}

bool CalcValRefPass::user_recursion(VertexPtr v, CalcValRefPass::LocalT *local, VisitVertex<CalcValRefPass> &visit) {
  for (VertexPtr &child : *v) {
    local->allowed = is_allowed_for_getting_val_or_ref(v->type(), &child == &v->back());
    visit(child);
  }
  return true;
}
