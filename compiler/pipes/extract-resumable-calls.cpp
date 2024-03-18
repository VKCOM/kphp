// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/extract-resumable-calls.h"

#include "compiler/compiler-core.h"
#include "compiler/inferring/public.h"
#include "compiler/name-gen.h"
#include "compiler/vertex-util.h"

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

  if (vk::any_of_equal(vertex->type(), op_ternary, op_log_or, op_log_and)) {
    for (auto v : *vertex) {
      if (is_resumable_expr(*skip_conv_and_sets(&v))) {
        return true;
      }
    }
  }

  if (auto null_coalesce = vertex.try_as<op_null_coalesce>()) {
    // only lhs is supported
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
    auto lhs_var = set_modify->lhs().try_as<op_var>();
    auto instance_prop = set_modify->lhs().try_as<op_instance_prop>();
    while (instance_prop) {
      lhs_var = instance_prop->instance().try_as<op_var>();
      instance_prop = instance_prop->instance().try_as<op_instance_prop>();
    }
    if (!lhs_var || lhs_var->var_id->is_in_global_scope()) {
      return {};
    }
  } else if (auto list = vertex.try_as<op_list>()) {
    resumable_expr = &list->array();
  } else if (auto set_value = vertex.try_as<op_set_value>()) {
    if (auto var_key = set_value->key().try_as<op_var>()) {
      if (!var_key->var_id->is_in_global_scope()) {
        resumable_expr = &set_value->value();
      }
    }
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

std::pair<VertexAdaptor<op_move>, VertexAdaptor<op_set>> ExtractResumableCallsPass::make_temp_resumable_var(VertexPtr init) noexcept {
  auto temp_var = VertexAdaptor<op_var>::create().set_location(init);
  temp_var->str_val = gen_unique_name("resumable_temp_var");
  temp_var->var_id = G->create_local_var(stage::get_function(), temp_var->str_val, VarData::var_local_t);
  temp_var->var_id->tinf_node.copy_type_from(tinf::get_type(init));
  auto set_op = VertexAdaptor<op_set>::create(temp_var.clone().set_rl_type(val_l), init).set_rl_type(val_none).set_location(init);
  temp_var->val_ref_flag = init->val_ref_flag;
  auto move_op = VertexAdaptor<op_move>::create(temp_var.set_rl_type(val_r)).set_rl_type(val_r).set_location(init);
  return {move_op, set_op};
}

VertexPtr ExtractResumableCallsPass::replace_set_ternary(VertexAdaptor<op_set_modify> set_vertex, VertexAdaptor<op_ternary> rhs_ternary) noexcept {
  auto set_true_case = set_vertex.clone();
  VertexPtr &true_case_rhs = *skip_conv_and_sets(&set_true_case->rhs());
  true_case_rhs = rhs_ternary->true_expr();
  true_case_rhs->val_ref_flag = rhs_ternary->val_ref_flag;

  auto set_false_case = set_vertex.clone();
  VertexPtr &false_case_rhs = *skip_conv_and_sets(&set_false_case->rhs());
  false_case_rhs = rhs_ternary->false_expr();
  false_case_rhs->val_ref_flag = rhs_ternary->val_ref_flag;

  auto ternary_replacer = VertexAdaptor<op_if>::create(rhs_ternary->cond(), VertexUtil::embrace(set_true_case), VertexUtil::embrace(set_false_case));
  return VertexUtil::embrace(ternary_replacer.set_rl_type(val_none).set_location(set_vertex));
}

VertexPtr ExtractResumableCallsPass::replace_set_logical_operation(VertexAdaptor<op_set_modify> set_vertex, VertexAdaptor<meta_op_binary> operation) noexcept {
  auto temp_var = make_temp_resumable_var(operation->lhs());

  auto set_if_lhs_true = set_vertex.clone();
  auto set_if_lhs_false = set_vertex.clone();

  switch (operation->type()) {
    case op_log_or:
      *skip_conv_and_sets(&set_if_lhs_true->rhs()) = temp_var.first;
      *skip_conv_and_sets(&set_if_lhs_false->rhs()) = operation->rhs();
      break;
    case op_log_and:
      *skip_conv_and_sets(&set_if_lhs_false->rhs()) = temp_var.first;
      *skip_conv_and_sets(&set_if_lhs_true->rhs()) = operation->rhs();
      break;
    default:
      kphp_assert(0 && "unexpected type");
  }

  auto check_lhs = VertexAdaptor<op_if>::create(temp_var.second, VertexUtil::embrace(set_if_lhs_true), VertexUtil::embrace(set_if_lhs_false));
  return VertexUtil::embrace(check_lhs.set_rl_type(val_none).set_location(operation));
}

VertexPtr ExtractResumableCallsPass::replace_resumable_expr_with_temp_var(VertexPtr *resumable_expr, VertexPtr expr_user) noexcept {
  auto temp_var = make_temp_resumable_var(*resumable_expr);
  *resumable_expr = temp_var.first;
  return VertexAdaptor<op_seq>::create(temp_var.second, expr_user).set_rl_type(val_none).set_location(expr_user);
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
    if (vk::any_of_equal((*resumable_expr)->type(), op_log_or, op_log_and)) {
      return replace_set_logical_operation(set_modify, resumable_expr->as<meta_op_binary>());
    }
  }

  return replace_resumable_expr_with_temp_var(resumable_expr, vertex);
}

bool ExtractResumableCallsPass::check_function(FunctionPtr function) const {
  return !function->is_extern() && function->is_resumable;
}
