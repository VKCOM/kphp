// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/extract-resumable-calls.h"

#include "compiler/compiler-core.h"
#include "compiler/gentree.h"
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

bool ExtractResumableCallsPass::is_resumable_expr(VertexPtr vertex) noexcept {
  if (auto func_call = vertex.try_as<op_func_call>()) {
    if (func_call->func_id->is_resumable) {
      return true;
    }
  }
  if (auto ternary = vertex.try_as<op_ternary>()) {
    for (auto v : *ternary) {
      if (is_resumable_expr(*skip_conv_and_sets(&v))) {
        return true;
      }
    }
  }
  if (auto null_coalesce = vertex.try_as<op_null_coalesce>()) {
    // support only lhs
    return is_resumable_expr(*skip_conv_and_sets(&null_coalesce->lhs()));
  }

  return false;
}

VertexPtr *ExtractResumableCallsPass::extract_resumable_expr(VertexPtr vertex) noexcept {
  VertexPtr *resumable_expr = nullptr;

  if (auto return_v = vertex.try_as<op_return>()) {
    resumable_expr = return_v->has_expr() ? &return_v->expr() : nullptr;
  } else if (auto set_modify = vertex.try_as<op_set_modify>()) {
    resumable_expr = &(set_modify->rhs());
    if ((*resumable_expr)->type() == op_func_call && vertex->type() == op_set && set_modify->lhs()->type() != op_instance_prop) {
      return {};
    }
  } else if (auto list = vertex.try_as<op_list>()) {
    resumable_expr = &list->array();
  } else if (auto set_value = vertex.try_as<op_set_value>()) {
    resumable_expr = &set_value->value();
  } else if (auto push_back = vertex.try_as<op_push_back>()) {
    resumable_expr = &push_back->value();
  } else if (auto if_cond = vertex.try_as<op_if>()) {
    resumable_expr = &if_cond->cond();
  }

  if (!resumable_expr || !*resumable_expr) {
    return {};
  }

  resumable_expr = skip_conv_and_sets(resumable_expr);
  if (!is_resumable_expr(*resumable_expr)) {
    return {};
  }
  return resumable_expr;
}

VertexAdaptor<op_var> ExtractResumableCallsPass::make_temp_resumable_var(const TypeData *type) noexcept {
  auto temp_var = VertexAdaptor<op_var>::create();
  temp_var->str_val = gen_unique_name("resumable_temp_var");
  temp_var->var_id = G->create_local_var(stage::get_function(), temp_var->str_val, VarData::var_local_t);
  temp_var->var_id->tinf_node.copy_type_from(type);
  return temp_var;
}

VertexPtr ExtractResumableCallsPass::replace_set_ternary(VertexAdaptor<op_set_modify> set_vertex, VertexAdaptor<op_ternary> rhs_ternary) noexcept {
  auto set_true_case = set_vertex.clone();
  *skip_conv_and_sets(&set_true_case->rhs()) = rhs_ternary->true_expr();

  auto set_false_case = set_vertex.clone();
  *skip_conv_and_sets(&set_false_case->rhs()) = rhs_ternary->false_expr();

  auto ternary_replacer = VertexAdaptor<op_if>::create(rhs_ternary->cond(), GenTree::embrace(set_true_case), GenTree::embrace(set_false_case));
  return GenTree::embrace(ternary_replacer.set_rl_type(val_none).set_location(set_vertex));
}

VertexPtr ExtractResumableCallsPass::replace_resumable_expr_with_temp_var(VertexPtr *resumable_expr, VertexPtr expr_user) noexcept {
  auto temp_var_with_res_of_func_call = make_temp_resumable_var(tinf::get_type(*resumable_expr)).set_rl_type(val_l).set_location(expr_user);
  auto set_op = VertexAdaptor<op_set>::create(temp_var_with_res_of_func_call, *resumable_expr).set_rl_type(val_none).set_location(expr_user);
  *resumable_expr = VertexAdaptor<op_move>::create(temp_var_with_res_of_func_call.clone().set_rl_type(val_r)).set_rl_type(val_r);
  resumable_expr->set_location(expr_user);
  return VertexAdaptor<op_seq>::create(set_op, expr_user).set_rl_type(val_none).set_location(expr_user);
}

VertexPtr ExtractResumableCallsPass::on_enter_vertex(VertexPtr vertex) {
  if (vertex->rl_type != val_none) {
    return vertex;
  }

  VertexPtr *resumable_expr = extract_resumable_expr(vertex);
  if (!resumable_expr) {
    return vertex;
  }

  if (auto set_modify = vertex.try_as<op_set_modify>()) {
    if (auto ternary = resumable_expr->try_as<op_ternary>()) {
      return replace_set_ternary(set_modify, ternary);
    }
    if (auto null_coalesce = resumable_expr->try_as<op_null_coalesce>()) {
      return replace_resumable_expr_with_temp_var(skip_conv_and_sets(&null_coalesce->lhs()), vertex);
    }
  }

  return replace_resumable_expr_with_temp_var(resumable_expr, vertex);
}

bool ExtractResumableCallsPass::check_function(FunctionPtr function) const {
  return !function->is_extern() && function->is_resumable;
}
