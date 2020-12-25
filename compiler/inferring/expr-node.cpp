// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/inferring/expr-node.h"

#include "compiler/compiler-core.h"
#include "compiler/data/define-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/var-data.h"
#include "compiler/gentree.h"
#include "compiler/inferring/node-recalc.h"

class ExprNodeRecalc : public NodeRecalc {
private:
  template<PrimitiveType tp>
  void recalc_ptype();
  template<typename InnerCall>
  void recalc_and_drop_false(InnerCall call);
  template<typename InnerCall>
  void recalc_and_drop_null(InnerCall call);

  void recalc_ternary(VertexAdaptor<op_ternary> ternary);
  void apply_type_rule_lca(VertexAdaptor<op_type_expr_lca> type_rule, VertexAdaptor<op_func_call> expr);
  void apply_type_rule_instance(VertexAdaptor<op_type_expr_instance> type_rule, VertexAdaptor<op_func_call> expr);
  void apply_type_rule_drop_or_false(VertexAdaptor<op_type_expr_drop_false> type_rule, VertexAdaptor<op_func_call> expr);
  void apply_type_rule_drop_or_null(VertexAdaptor<op_type_expr_drop_null> type_rule, VertexAdaptor<op_func_call> expr);
  void apply_type_rule_callback_call(VertexAdaptor<op_type_expr_callback_call> type_rule, VertexAdaptor<op_func_call> expr);
  void apply_type_rule_callable(VertexAdaptor<op_type_expr_callable> type_rule, VertexAdaptor<op_func_call> expr);
  void apply_type_rule_type(VertexAdaptor<op_type_expr_type> rule, VertexAdaptor<op_func_call> expr);
  void apply_arg_ref(VertexAdaptor<op_type_expr_arg_ref> arg, VertexPtr expr);
  void apply_instance_arg_ref(VertexAdaptor<op_type_expr_arg_ref> arg, VertexPtr expr);
  void apply_index(VertexAdaptor<op_index> index, VertexAdaptor<op_func_call> expr);
  void apply_type_rule(VertexPtr rule, VertexAdaptor<op_func_call> expr);
  void recalc_func_call(VertexAdaptor<op_func_call> call);
  void recalc_var(VertexAdaptor<op_var> var);
  void recalc_push_back_return(VertexAdaptor<op_push_back_return> pb);
  void recalc_index(VertexAdaptor<op_index> index);
  void recalc_instance_prop(VertexAdaptor<op_instance_prop> index);
  void recalc_set(VertexAdaptor<op_set> set);
  void recalc_double_arrow(VertexAdaptor<op_double_arrow> arrow);
  void recalc_foreach_param(VertexAdaptor<op_foreach_param> param);
  void recalc_conv_array(VertexAdaptor<meta_op_unary> conv);
  void recalc_array(VertexAdaptor<op_array> array);
  void recalc_tuple(VertexAdaptor<op_tuple> tuple);
  void recalc_shape(VertexAdaptor<op_shape> shape);
  void recalc_plus_minus(VertexAdaptor<meta_op_unary> expr);
  void recalc_inc_dec(VertexAdaptor<meta_op_unary> expr);
  void recalc_noerr(VertexAdaptor<op_noerr> expr);
  void recalc_arithm(VertexAdaptor<meta_op_binary> expr);
  void recalc_power(VertexAdaptor<op_pow> expr);
  void recalc_fork(VertexAdaptor<op_fork> fork);
  void recalc_null_coalesce(VertexAdaptor<op_null_coalesce> expr);
  void recalc_return(VertexAdaptor<op_return> expr);
  void recalc_expr(VertexPtr expr);

public:
  ExprNodeRecalc(tinf::ExprNode *node, tinf::TypeInferer *inferer);
  void do_recalc() final;
  bool auto_edge_flag() final;
};

template<PrimitiveType tp>
void ExprNodeRecalc::recalc_ptype() {

  set_lca(TypeData::get_type(tp));
}

void ExprNodeRecalc::recalc_ternary(VertexAdaptor<op_ternary> ternary) {
  set_lca(ternary->true_expr());
  set_lca(ternary->false_expr());
}

void ExprNodeRecalc::apply_type_rule_lca(VertexAdaptor<op_type_expr_lca> type_rule, VertexAdaptor<op_func_call> expr) {
  for (auto i : type_rule->args()) {
    //TODO: is it hack?
    apply_type_rule(i, expr);
  }
}

void ExprNodeRecalc::apply_type_rule_instance(VertexAdaptor<op_type_expr_instance> type_rule, VertexAdaptor<op_func_call> expr) {
  apply_instance_arg_ref(type_rule->expr().try_as<op_type_expr_arg_ref>(), expr);
}

template<typename InnerCall>
void ExprNodeRecalc::recalc_and_drop_false(InnerCall call) {
  push_type();
  call();
  TypeData *type = pop_type();
  set_lca(drop_or_false(as_rvalue(type)));
}

template<typename InnerCall>
void ExprNodeRecalc::recalc_and_drop_null(InnerCall call) {
  push_type();
  call();
  TypeData *type = pop_type();
  set_lca(drop_or_null(as_rvalue(type)));
}

void ExprNodeRecalc::apply_type_rule_drop_or_false(VertexAdaptor<op_type_expr_drop_false> type_rule, VertexAdaptor<op_func_call> expr) {
  return recalc_and_drop_false([&] { apply_type_rule(type_rule->expr(), expr); });
}

void ExprNodeRecalc::apply_type_rule_drop_or_null(VertexAdaptor<op_type_expr_drop_null> type_rule, VertexAdaptor<op_func_call> expr) {
  return recalc_and_drop_null([&] { apply_type_rule(type_rule->expr(), expr); });
}

void ExprNodeRecalc::apply_type_rule_callback_call(VertexAdaptor<op_type_expr_callback_call> type_rule, VertexAdaptor<op_func_call> expr) {
  auto arg = type_rule->expr().as<op_type_expr_arg_ref>();
  int callback_arg_id = GenTree::get_id_arg_ref(arg, expr);
  if (callback_arg_id == -1) {
    kphp_error (0, "error in type rule");
    recalc_ptype<tp_Error>();
  }
  auto called_function = expr->func_id;
  kphp_assert(called_function->is_extern() && called_function->get_params()[callback_arg_id]->type() == op_func_param_typed_callback);

  VertexRange call_args = expr->args();
  VertexPtr callback_arg = call_args[callback_arg_id];

  if (auto ptr = callback_arg.try_as<op_func_ptr>()) {
    if (auto rule = ptr->func_id->root->type_rule) {
      VertexPtr son_rule = rule->rule();
      while (son_rule->size() == 1) {
        if (auto type_expr = son_rule.try_as<op_type_expr_type>()) {
          son_rule = type_expr->args()[0];
        } else {
          recalc_ptype<tp_Error>();
          break;
        };
      }
      if (son_rule->type() == op_type_expr_type && son_rule->empty()) {
        apply_type_rule(rule->rule(), expr);
        return;
      }
      recalc_ptype<tp_Error>();
    }
    set_lca(ptr->func_id, -1);
  } else {
    recalc_ptype<tp_Error>();
  }
}

void ExprNodeRecalc::apply_type_rule_callable(VertexAdaptor<op_type_expr_callable> type_rule __attribute__ ((unused)), VertexAdaptor<op_func_call> expr __attribute__ ((unused))) {
  // the 'callable' keyword used in phpdoc/type declaration doesn't affect the type inference
  // (but it does imply a template parameter for it)
}

void ExprNodeRecalc::apply_type_rule_type(VertexAdaptor<op_type_expr_type> rule, VertexAdaptor<op_func_call> expr) {
  // it could be just set_lca(rule->type_help), but we don't allow a void|null combination
  if (rule->type_help == tp_Null) {
    if (new_type()->ptype() != tp_void) {
      recalc_ptype<tp_Null>();
    }
  } else {
    set_lca(rule->type_help);
  }

  if (!rule->empty()) {
    switch (rule->type_help) {
      case tp_array:
      case tp_future:
      case tp_future_queue: {
        push_type();
        apply_type_rule(rule->args()[0], expr);
        TypeData *tmp = pop_type();
        set_lca_at(&MultiKey::any_key(1), as_rvalue(tmp));
        delete tmp;
        break;
      }
      case tp_tuple: {
        for (int int_index = 0; int_index < rule->size(); ++int_index) {
          push_type();
          apply_type_rule(rule->args()[int_index], expr);
          TypeData *tmp = pop_type();
          MultiKey key({Key::int_key(int_index)});
          set_lca_at(&key, as_rvalue(tmp));
          delete tmp;
        }
        break;
      }
      case tp_shape: {
        for (const auto &elem : rule->args()) {
          auto double_arrow = elem.as<op_double_arrow>();
          push_type();
          apply_type_rule(double_arrow->rhs(), expr);
          TypeData *tmp = pop_type();
          MultiKey key({Key::string_key(double_arrow->lhs()->get_string())});
          set_lca_at(&key, as_rvalue(tmp));
          delete tmp;
        }
        if (rule->extra_type == op_ex_shape_has_varg) {
          new_type_->set_shape_has_varg_flag();
        }
        break;
      }
      default:
        kphp_fail();
    }
  }
}

void ExprNodeRecalc::apply_arg_ref(VertexAdaptor<op_type_expr_arg_ref> arg, VertexPtr expr) {
  if (GenTree::get_id_arg_ref(arg, expr) == -1) {
    kphp_error (0, "error in type rule");
    recalc_ptype<tp_Error>();
  }

  if (auto vertex = GenTree::get_call_arg_ref(arg, expr)) {
    set_lca(vertex);
  }
}

void ExprNodeRecalc::apply_instance_arg_ref(VertexAdaptor<op_type_expr_arg_ref> arg, VertexPtr expr) {
  std::string err_msg;

  if (auto vertex = GenTree::get_call_arg_ref(arg, expr)) {
    auto class_name = GenTree::get_constexpr_string(vertex);

    if (class_name && !class_name->empty()) {
      if (auto klass = G->get_class(*class_name)) {
        set_lca(klass);
      } else {
        err_msg = fmt_format("bad {} parameter: can't find class {}", arg->int_val, *class_name);
      }
    } else {
      err_msg = fmt_format("bad {} parameter: expected constant nonempty string with class name", arg->int_val);
    }
  } else {
    err_msg = "error in type rule";
  }

  if (!err_msg.empty()) {
    kphp_error(false, err_msg);
    recalc_ptype<tp_Error>();
  }
}

void ExprNodeRecalc::apply_index(VertexAdaptor<op_index> index, VertexAdaptor<op_func_call> expr) {
  push_type();
  apply_type_rule(index->array(), expr);
  TypeData *type = pop_type();
  set_lca(type, &MultiKey::any_key(1));
  delete type;
}

void ExprNodeRecalc::apply_type_rule(VertexPtr rule, VertexAdaptor<op_func_call> expr) {
  switch (rule->type()) {
    case op_type_expr_lca:
      apply_type_rule_lca(rule.as<op_type_expr_lca>(), expr);
      break;
    case op_type_expr_instance:
      apply_type_rule_instance(rule.as<op_type_expr_instance>(), expr);
      break;
    case op_type_expr_drop_false:
      apply_type_rule_drop_or_false(rule.as<op_type_expr_drop_false>(), expr);
      break;
    case op_type_expr_drop_null:
      apply_type_rule_drop_or_null(rule.as<op_type_expr_drop_null>(), expr);
      break;
    case op_type_expr_callback_call:
      apply_type_rule_callback_call(rule.as<op_type_expr_callback_call>(), expr);
      break;
    case op_type_expr_callable:
      apply_type_rule_callable(rule.as<op_type_expr_callable>(), expr);
      break;
    case op_type_expr_type:
      apply_type_rule_type(rule.as<op_type_expr_type>(), expr);
      break;
    case op_type_expr_arg_ref:
      apply_arg_ref(rule.as<op_type_expr_arg_ref>(), expr);
      break;
    case op_index:
      apply_index(rule.as<op_index>(), expr);
      break;
    case op_type_expr_class:
      set_lca(rule.as<op_type_expr_class>()->class_ptr);
      break;
    default:
      kphp_error (0, "error in type rule");
      recalc_ptype<tp_Error>();
      break;
  }
}

void ExprNodeRecalc::recalc_func_call(VertexAdaptor<op_func_call> call) {
  FunctionPtr function = call->func_id;
  if (call->type_rule) {
    apply_type_rule(call->type_rule->rule(), call);
    if (call->type_rule->type() == op_common_type_rule) {
      return;
    }
  }

  if (function->root->type_rule) {
    apply_type_rule(function->root->type_rule->rule(), call);
  } else {
    set_lca(function, -1);
  }
}

void ExprNodeRecalc::recalc_var(VertexAdaptor<op_var> var) {
  set_lca(var->var_id);
}

void ExprNodeRecalc::recalc_push_back_return(VertexAdaptor<op_push_back_return> pb) {
  set_lca(pb->array(), &MultiKey::any_key(1));
}

void ExprNodeRecalc::recalc_index(VertexAdaptor<op_index> index) {
  bool is_const_int_index = index->has_key() && GenTree::get_actual_value(index->key())->type() == op_int_const;
  if (is_const_int_index) {
    long int_index = parse_int_from_string(GenTree::get_actual_value(index->key()).as<op_int_const>());
    MultiKey key({Key::int_key((int)int_index)});
    set_lca(index->array(), &key);
    return;
  }

  bool is_const_string_index = index->has_key() && GenTree::get_actual_value(index->key())->type() == op_string;
  if (is_const_string_index) {
    MultiKey key({Key::string_key(GenTree::get_actual_value(index->key())->get_string())});
    set_lca(index->array(), &key);
    return;
  }

  set_lca(index->array(), &MultiKey::any_key(1));
}

void ExprNodeRecalc::recalc_instance_prop(VertexAdaptor<op_instance_prop> index) {
  set_lca(index->var_id);
}

void ExprNodeRecalc::recalc_set(VertexAdaptor<op_set> set) {
  set_lca(set->lhs());
}

void ExprNodeRecalc::recalc_double_arrow(VertexAdaptor<op_double_arrow> arrow) {
  set_lca(arrow->value());
}

void ExprNodeRecalc::recalc_foreach_param(VertexAdaptor<op_foreach_param> param) {
  set_lca(param->xs(), &MultiKey::any_key(1));
}

void ExprNodeRecalc::recalc_conv_array(VertexAdaptor<meta_op_unary> conv) {
  VertexPtr arg = conv->expr();
  //FIXME: (extra dependenty)
  add_dependency(as_rvalue(arg));
  if (tinf::get_type(arg)->get_real_ptype() == tp_array) {
    set_lca(drop_optional(as_rvalue(arg)));
    // foreach/array_map/(array) on tuples and instances is an error
  } else if (vk::any_of_equal(tinf::get_type(arg)->ptype(), tp_tuple, tp_shape, tp_Class)) {
    set_lca(TypeData::get_type(tp_Error));
  } else {
    recalc_ptype<tp_array>();
    if (tinf::get_type(arg)->ptype() != tp_any) { //hack
      set_lca_at(&MultiKey::any_key(1), tinf::get_type(arg)->get_real_ptype());
    }
  }
}

void ExprNodeRecalc::recalc_array(VertexAdaptor<op_array> array) {
  recalc_ptype<tp_array>();
  for (auto i : array->args()) {
    set_lca_at(&MultiKey::any_key(1), i);
  }
}

void ExprNodeRecalc::recalc_tuple(VertexAdaptor<op_tuple> tuple) {
  recalc_ptype<tp_tuple>();
  int index = 0;
  for (auto i: tuple->args()) {
    vector<Key> i_key_index{Key::int_key(index++)};
    MultiKey key(i_key_index);
    set_lca_at(&key, i);
  }
}

void ExprNodeRecalc::recalc_shape(VertexAdaptor<op_shape> shape) {
  recalc_ptype<tp_shape>();
  for (auto i: shape->args()) {
    auto double_arrow = i.as<op_double_arrow>();
    const std::string &str_index = GenTree::get_actual_value(double_arrow->key())->get_string();
    vector<Key> i_key_index{Key::string_key(str_index)};
    MultiKey key(i_key_index);
    set_lca_at(&key, double_arrow->value());
  }
}

void ExprNodeRecalc::recalc_plus_minus(VertexAdaptor<meta_op_unary> expr) {
  set_lca(drop_optional(as_rvalue(expr->expr())));
  if (new_type()->ptype() == tp_string) {
    recalc_ptype<tp_mixed>();
  }
}

void ExprNodeRecalc::recalc_inc_dec(VertexAdaptor<meta_op_unary> expr) {
  //or false ???
  set_lca(drop_optional(as_rvalue(expr->expr())));
}

void ExprNodeRecalc::recalc_noerr(VertexAdaptor<op_noerr> expr) {
  set_lca(as_rvalue(expr->expr()));
}

void ExprNodeRecalc::recalc_fork(VertexAdaptor<op_fork> fork) {
  recalc_ptype<tp_future>();
  set_lca_at(&MultiKey::any_key(1), fork->func_call());
}

void ExprNodeRecalc::recalc_null_coalesce(VertexAdaptor<op_null_coalesce> expr) {
  set_lca(drop_or_null(as_rvalue(expr->lhs())));
  set_lca(as_rvalue(expr->rhs()));
}

void ExprNodeRecalc::recalc_return(VertexAdaptor<op_return> expr) {
  if (expr->has_expr()) {
    set_lca(expr->expr());
  } else {
    recalc_ptype<tp_void>();
  }
}

void ExprNodeRecalc::recalc_arithm(VertexAdaptor<meta_op_binary> expr) {
  VertexPtr lhs = expr->lhs();
  VertexPtr rhs = expr->rhs();

  //FIXME: (extra dependency)
  add_dependency(as_rvalue(lhs));
  add_dependency(as_rvalue(rhs));

  if (tinf::get_type(lhs)->ptype() == tp_bool) {
    recalc_ptype<tp_int>();
  } else {
    set_lca(drop_optional(as_rvalue(lhs)));
  }

  if (tinf::get_type(rhs)->ptype() == tp_bool) {
    recalc_ptype<tp_int>();
  } else {
    set_lca(drop_optional(as_rvalue(rhs)));
  }

  if (new_type()->ptype() == tp_string) {
    recalc_ptype<tp_mixed>();
  }
}

void ExprNodeRecalc::recalc_power(VertexAdaptor<op_pow> expr) {
  VertexPtr base = expr->lhs();
  add_dependency(as_rvalue(base));
  VertexPtr exponent = expr->rhs();
  if (is_positive_constexpr_int(exponent)) {
    recalc_ptype<tp_int>();
    set_lca(drop_optional(as_rvalue(base)));
  } else {
    recalc_ptype<tp_mixed>();
  }
}

void ExprNodeRecalc::recalc_expr(VertexPtr expr) {
  switch (expr->type()) {
    case op_move:
      recalc_expr(expr.as<op_move>()->expr());
      break;
    case op_ternary:
      recalc_ternary(expr.as<op_ternary>());
      break;
    case op_func_call:
      recalc_func_call(expr.as<op_func_call>());
      break;
    case op_exception_constructor_call:
      recalc_func_call(expr.as<op_exception_constructor_call>()->constructor_call());
      break;
    case op_common_type_rule:
    case op_gt_type_rule:
    case op_lt_type_rule:
    case op_set_check_type_rule:
      apply_type_rule(expr.as<meta_op_type_rule>()->rule(), VertexAdaptor<op_func_call>{});
      break;
    case op_var:
      recalc_var(expr.as<op_var>());
      break;
    case op_push_back_return:
      recalc_push_back_return(expr.as<op_push_back_return>());
      break;
    case op_index:
      recalc_index(expr.as<op_index>());
      break;
    case op_instance_prop:
      recalc_instance_prop(expr.as<op_instance_prop>());
      break;
    case op_set:
      recalc_set(expr.as<op_set>());
      break;
    case op_false:
      recalc_ptype<tp_False>();
      break;
    case op_log_or_let:
    case op_log_and_let:
    case op_log_xor_let:
    case op_log_or:
    case op_log_and:
    case op_log_not:
    case op_conv_bool:
    case op_true:
    case op_eq2:
    case op_eq3:
    case op_neq2:
    case op_neq3:
    case op_lt:
    case op_gt:
    case op_le:
    case op_ge:
    case op_isset:
      recalc_ptype<tp_bool>();
      break;

    case op_conv_string:
    case op_conv_string_l:
    case op_concat:
    case op_string_build:
    case op_string:
      recalc_ptype<tp_string>();
      break;

    case op_conv_int:
    case op_conv_int_l:
    case op_int_const:
    case op_mod:
    case op_spaceship:
    case op_not:
    case op_or:
    case op_and:
    case op_xor:
      recalc_ptype<tp_int>();
      break;
    case op_fork:
      recalc_fork(expr.as<op_fork>());
      break;

    case op_conv_float:
    case op_float_const:
    case op_div:
      recalc_ptype<tp_float>();
      break;

    case op_conv_regexp:
      recalc_ptype<tp_regexp>();
      break;

    case op_double_arrow:
      recalc_double_arrow(expr.as<op_double_arrow>());
      break;

    case op_foreach_param:
      recalc_foreach_param(expr.as<op_foreach_param>());
      break;

    case op_conv_array:
    case op_conv_array_l:
      recalc_conv_array(expr.as<meta_op_unary>());
      break;

    case op_array:
      recalc_array(expr.as<op_array>());
      break;
    case op_tuple:
      recalc_tuple(expr.as<op_tuple>());
      break;
    case op_shape:
      recalc_shape(expr.as<op_shape>());
      break;

    case op_conv_mixed:
      // we don't want to spoil types and need this vertex just for further checks
      recalc_expr(expr.as<op_conv_mixed>()->expr());
      break;

    case op_force_mixed:
      recalc_ptype<tp_mixed>();
      break;

    case op_null:
      recalc_ptype<tp_Null>();
      break;

    case op_conv_drop_false: {
      return recalc_and_drop_false([&] { recalc_expr(expr.as<op_conv_drop_false>()->expr());});
    }
    case op_conv_drop_null: {
      return recalc_and_drop_null([&] { recalc_expr(expr.as<op_conv_drop_null>()->expr());});
    }

    case op_plus:
    case op_minus:
      recalc_plus_minus(expr.as<meta_op_unary>());
      break;

    case op_prefix_inc:
    case op_prefix_dec:
    case op_postfix_inc:
    case op_postfix_dec:
      recalc_inc_dec(expr.as<meta_op_unary>());
      break;

    case op_noerr:
      recalc_noerr(expr.as<op_noerr>());
      break;

    case op_sub:
    case op_add:
    case op_mul:
    case op_shl:
    case op_shr:
      recalc_arithm(expr.as<meta_op_binary>());
      break;

    case op_pow:
      recalc_power(expr.as<op_pow>());
      break;

    case op_clone:
      set_lca(expr.as<op_clone>()->expr());
      break;

    case op_seq_rval:
      set_lca(expr.as<op_seq_rval>()->back());
      break;

    case op_null_coalesce:
      recalc_null_coalesce(expr.as<op_null_coalesce>());
      break;

    case op_return:
      recalc_return(expr.as<op_return>());
      break;

    case op_alloc:
      set_lca(expr.as<op_alloc>()->allocated_class);
      break;

    case op_instanceof:
      recalc_ptype<tp_bool>();
      break;

    default:
      recalc_ptype<tp_mixed>();
      break;
  }
}

bool ExprNodeRecalc::auto_edge_flag() {
  // on the very first recalc, add all dependent edges while calculating lca
  return !node_->was_recalc_finished_at_least_once();
}

ExprNodeRecalc::ExprNodeRecalc(tinf::ExprNode *node, tinf::TypeInferer *inferer) :
  NodeRecalc(node, inferer) {
}

void ExprNodeRecalc::do_recalc() {
  //fprintf (stderr, "recalc expr %d %p %s\n", get_thread_id(), node_, node_->get_description().c_str());
  VertexPtr expr = dynamic_cast<tinf::ExprNode *>(node_)->get_expr();
  stage::set_location(expr->get_location());
  recalc_expr(expr);
}

void tinf::ExprNode::recalc(tinf::TypeInferer *inferer) {
  ExprNodeRecalc(this, inferer).run();
}

std::string tinf::ExprNode::convert_expr_to_human_readable(VertexPtr expr) {
  switch (expr->type()) {
    case op_var:
      return expr.as<op_var>()->var_id->get_human_readable_name();

    case op_func_call: {
      auto func = expr.as<op_func_call>()->func_id;
      return func->is_constructor() ? "new " + func->class_id->name : func->get_human_readable_name();
    }

    case op_instance_prop:
      return convert_expr_to_human_readable(expr.as<op_instance_prop>()->instance()) + "->" + expr->get_string();

    case op_index: {
      string suff;
      auto outer_expr = expr;
      while (outer_expr->type() == op_index) {
        suff += "[.]";
        outer_expr = outer_expr.as<op_index>()->array();
      }
      return convert_expr_to_human_readable(outer_expr) + suff;
    }

    case op_int_const:
    case op_float_const:
      return expr->get_string();
    case op_string:
      return '"' + expr->get_string() + '"';

    default:
      return OpInfo::str(expr->type());
  }
}

std::string tinf::ExprNode::get_expr_human_readable() const {
  return convert_expr_to_human_readable(expr_);
}

std::string tinf::ExprNode::get_description() {
  return "ExprNode " + get_expr_human_readable() + " at " + get_location().as_human_readable() + " : " + tinf::get_type(expr_)->as_human_readable();
}

const Location &tinf::ExprNode::get_location() const {
  return expr_->location;
}
