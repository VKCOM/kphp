#include "compiler/pipes/extract-resumable-calls.h"

#include "compiler/compiler-core.h"
#include "compiler/inferring/public.h"
#include "compiler/name-gen.h"

void ExtractResumableCallsPass::skip_conv_and_sets(VertexPtr *&replace) {
  while (true) {
    if (!replace) {
      break;
    }
    Operation op = (*replace)->type();
    if (op == op_set_add || op == op_set_sub ||
        op == op_set_mul || op == op_set_div ||
        op == op_set_mod || op == op_set_and ||
        op == op_set_or || op == op_set_xor ||
        op == op_set_dot || op == op_set ||
        op == op_set_shr || op == op_set_shl) {
      replace = &((*replace).as<meta_op_binary>()->rhs());
    } else if (op == op_conv_int || op == op_conv_bool ||
               op == op_conv_int_l || op == op_conv_float ||
               op == op_conv_string || op == op_conv_array ||
               op == op_conv_array_l || op == op_conv_var ||
               op == op_conv_uint || op == op_conv_long ||
               op == op_conv_ulong || op == op_conv_regexp ||
               op == op_log_not) {
      replace = &((*replace).as<meta_op_unary>()->expr());
    } else {
      break;
    }
  }
}
VertexPtr ExtractResumableCallsPass::on_enter_vertex(VertexPtr vertex, ExtractResumableCallsPass::LocalT *local) {
  if (local->from_seq == false) {
    return vertex;
  }
  VertexPtr *replace = nullptr;
  VertexAdaptor<op_func_call> func_call;
  Operation op = vertex->type();
  if (op == op_return) {
    replace = &vertex.as<op_return>()->expr();
  } else if (op == op_set_add || op == op_set_sub ||
             op == op_set_mul || op == op_set_div ||
             op == op_set_mod || op == op_set_and ||
             op == op_set_or || op == op_set_xor ||
             op == op_set_dot || op == op_set ||
             op == op_set_shr || op == op_set_shl) {
    replace = &vertex.as<meta_op_binary>()->rhs();
    if ((*replace)->type() == op_func_call && op == op_set) {
      return vertex;
    }
  } else if (op == op_list) {
    replace = &vertex.as<op_list>()->array();
  } else if (op == op_set_value) {
    replace = &vertex.as<op_set_value>()->value();
  } else if (op == op_push_back) {
    replace = &vertex.as<op_push_back>()->value();
  } else if (op == op_if) {
    replace = &vertex.as<op_if>()->cond();
  }
  skip_conv_and_sets(replace);
  if (!replace || !*replace || (*replace)->type() != op_func_call) {
    return vertex;
  }
  func_call = *replace;
  FunctionPtr func = func_call->get_func_id();
  if (func->root->resumable_flag == false) {
    return vertex;
  }
  auto temp_var = VertexAdaptor<op_var>::create();
  temp_var->str_val = gen_unique_name("resumable_temp_var");
  VarPtr var = G->create_local_var(stage::get_function(), temp_var->str_val, VarData::var_local_t);
  var->tinf_node.copy_type_from(tinf::get_type(func, -1));
  temp_var->set_var_id(var);
  auto set_op = VertexAdaptor<op_set>::create(temp_var, func_call);

  auto temp_var2 = VertexAdaptor<op_var>::create();
  temp_var2->str_val = temp_var->str_val;
  temp_var2->set_var_id(var);
  *replace = temp_var2;

  auto seq = VertexAdaptor<op_seq>::create(set_op, vertex);
  return seq;
}
void ExtractResumableCallsPass::on_enter_edge(VertexPtr vertex, LocalT *, VertexPtr, LocalT *dest_local) {
  dest_local->from_seq = vertex->type() == op_seq || vertex->type() == op_seq_rval;
}
bool ExtractResumableCallsPass::check_function(FunctionPtr function) {
  return default_check_function(function) && function->type() != FunctionData::func_extern &&
         function->root->resumable_flag;
}
