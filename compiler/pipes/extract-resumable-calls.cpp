// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/extract-resumable-calls.h"

#include "compiler/compiler-core.h"
#include "compiler/inferring/public.h"
#include "compiler/name-gen.h"

VertexPtr *ExtractResumableCallsPass::skip_conv_and_sets(VertexPtr *replace) noexcept {
  if (auto set_modify = replace->try_as<op_set_modify>()) {
    return skip_conv_and_sets(&(set_modify->rhs()));
  }
  if (OpInfo::type((*replace)->type()) == conv_op || vk::any_of_equal((*replace)->type(), op_conv_int_l, op_conv_string_l, op_conv_array_l, op_log_not)) {
    return skip_conv_and_sets(&((*replace).as<meta_op_unary>()->expr()));
  }
  return replace;
}

VertexPtr ExtractResumableCallsPass::try_save_resumable_func_call_in_temp_var(VertexPtr vertex) noexcept {
  VertexPtr *resumable_func_call = nullptr;

  if (auto return_v = vertex.try_as<op_return>()) {
    resumable_func_call = return_v->has_expr() ? &return_v->expr() : nullptr;
  } else if (auto set_modify = vertex.try_as<op_set_modify>()) {
    resumable_func_call = &(set_modify->rhs());

    if ((*resumable_func_call)->type() == op_func_call && vertex->type() == op_set && set_modify->lhs()->type() != op_instance_prop) {
      return {};
    }
  } else if (auto list = vertex.try_as<op_list>()) {
    resumable_func_call = &list->array();
  } else if (auto set_value = vertex.try_as<op_set_value>()) {
    resumable_func_call = &set_value->value();
  } else if (auto push_back = vertex.try_as<op_push_back>()) {
    resumable_func_call = &push_back->value();
  } else if (auto if_cond = vertex.try_as<op_if>()) {
    resumable_func_call = &if_cond->cond();
  }

  if (!resumable_func_call || !*resumable_func_call) {
    return {};
  }

  resumable_func_call = skip_conv_and_sets(resumable_func_call);
  auto func_call = resumable_func_call->try_as<op_func_call>();
  if (!func_call || !func_call->func_id->is_resumable) {
    return {};
  }

  auto temp_var_with_res_of_func_call = make_temp_resumable_var(tinf::get_type(func_call)).set_rl_type(val_l).set_location(vertex);
  auto set_op = VertexAdaptor<op_set>::create(temp_var_with_res_of_func_call, func_call).set_rl_type(val_none).set_location(vertex);;
  *resumable_func_call = VertexAdaptor<op_move>::create(temp_var_with_res_of_func_call.clone().set_rl_type(val_r)).set_rl_type(val_r);
  resumable_func_call->set_location(vertex);
  return VertexAdaptor<op_seq>::create(set_op, vertex).set_rl_type(val_none).set_location(vertex);
}

VertexAdaptor<op_var> ExtractResumableCallsPass::make_temp_resumable_var(const TypeData *type) noexcept {
  auto temp_var = VertexAdaptor<op_var>::create();
  temp_var->str_val = gen_unique_name("resumable_temp_var");
  temp_var->var_id = G->create_local_var(stage::get_function(), temp_var->str_val, VarData::var_local_t);
  temp_var->var_id->tinf_node.copy_type_from(type);
  return temp_var;
}

VertexPtr ExtractResumableCallsPass::on_enter_vertex(VertexPtr vertex) {
  if (vertex->rl_type != val_none) {
    return vertex;
  }

  if (auto new_vertex = try_save_resumable_func_call_in_temp_var(vertex)) {
    return new_vertex;
  }

  return vertex;
}

bool ExtractResumableCallsPass::check_function(FunctionPtr function) const {
  return !function->is_extern() && function->is_resumable;
}
