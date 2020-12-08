// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/calc-val-ref.h"

#include "compiler/inferring/public.h"

bool CalcValRefPass::is_allowed_for_getting_val_or_ref(Operation op, bool is_last, bool is_first) {
  switch (op) {
    case op_push_back:
    case op_push_back_return:
    case op_index:
      return !is_last;

    case op_set_value:
      return is_first;

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

    case op_conv_int:
    case op_conv_int_l:
    case op_conv_float:
    case op_conv_string_l:
      // case op_conv_bool is ignored on purpose so f$boolval(Optional<T>) doesn't turn into the default T

    case op_unset:
    case op_switch:
      return true;

    default:
      return false;
  }
}
VertexPtr CalcValRefPass::on_enter_vertex(VertexPtr v) {
  for (auto child_it = v->begin(); child_it != v->end(); ++child_it) {
    bool allowed = is_allowed_for_getting_val_or_ref(v->type(), child_it == std::prev(v->end()), child_it == v->begin());
    VertexPtr dest_vertex = *child_it;
    if (allowed && dest_vertex->rl_type != val_none && dest_vertex->rl_type != val_error) {
      if (tinf::get_type(dest_vertex)->use_optional()) {
        dest_vertex->val_ref_flag = dest_vertex->rl_type;
      }
    }
  }
  return v;
}
