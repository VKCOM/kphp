#include "compiler/pipes/inline-simple-functions.h"

VertexPtr InlineSimpleFunctions::on_enter_vertex(VertexPtr root, FunctionPassBase::LocalT *) {
  if (inline_is_possible_) {
    switch (root->type()) {
      case op_function:
      case op_func_name:
      case op_func_param:
      case op_func_call:
      case op_instance_prop:
      case op_empty:
      case op_var:
      case op_return:
      case op_set:
      case op_set_add:
      case op_set_sub:
      case op_set_mul:
      case op_set_div:
      case op_set_mod:
      case op_set_pow:
      case op_set_and:
      case op_set_or:
      case op_set_xor:
      case op_set_dot:
      case op_set_shr:
      case op_set_shl:
      case op_add:
      case op_sub:
      case op_int_const:
      case op_float_const:
        break;
      case op_func_param_list:
      case op_seq:
        if (root->size() > 5) {
          inline_is_possible_ = false;
        }
        break;
      default:
        inline_is_possible_ = false;
    }
  }
  return root;
}

bool InlineSimpleFunctions::check_function(FunctionPtr function) {
  return default_check_function(function) &&
         !function->is_resumable &&
         !function->is_inline;
}

nullptr_t InlineSimpleFunctions::on_finish() {
  if (inline_is_possible_) {
    current_function->is_inline = true;
  }
  return FunctionPassBase::on_finish();
}
