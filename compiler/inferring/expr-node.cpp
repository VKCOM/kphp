// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/inferring/expr-node.h"

#include "common/php-functions.h"

#include "compiler/compiler-core.h"
#include "compiler/data/define-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/var-data.h"
#include "compiler/inferring/edge.h"
#include "compiler/inferring/node-recalc.h"
#include "compiler/type-hint.h"
#include "compiler/vertex-util.h"

class ExprNodeRecalc : public NodeRecalc {
private:
  template<PrimitiveType tp>
  void recalc_ptype();

  void recalc_ffi_addr(VertexAdaptor<op_ffi_addr> addr);
  void recalc_php2c(VertexAdaptor<op_ffi_php2c_conv> conv);
  void recalc_c2php(VertexAdaptor<op_ffi_c2php_conv> conv);
  void recalc_ternary(VertexAdaptor<op_ternary> ternary);
  void recalc_func_call(VertexAdaptor<op_func_call> call);
  void recalc_arg_ref_rule(VertexAdaptor<op_trigger_recalc_arg_ref_rule> expr);
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
  void recalc_and_drop_false(VertexAdaptor<op_conv_drop_false> expr);
  void recalc_and_drop_null(VertexAdaptor<op_conv_drop_null> expr);
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

void ExprNodeRecalc::recalc_and_drop_false(VertexAdaptor<op_conv_drop_false> expr) {
  VertexPtr inner_expr = expr->expr();
  inner_expr->tinf_node.recalc(inferer_);
  set_lca(drop_or_false(as_rvalue(inner_expr)));
}

void ExprNodeRecalc::recalc_and_drop_null(VertexAdaptor<op_conv_drop_null> expr) {
  VertexPtr inner_expr = expr->expr();
  inner_expr->tinf_node.recalc(inferer_);
  set_lca(drop_or_null(as_rvalue(inner_expr)));
}

void ExprNodeRecalc::recalc_ffi_addr(VertexAdaptor<op_ffi_addr> addr) {
  addr->expr()->tinf_node.recalc(inferer_);
  set_lca(ffi_rvalue_take_addr(ffi_rvalue_drop_ref(as_rvalue(addr->expr()))));
}

void ExprNodeRecalc::recalc_php2c(VertexAdaptor<op_ffi_php2c_conv> conv) {
  set_lca(conv->c_type->to_type_data());
}

void ExprNodeRecalc::recalc_c2php(VertexAdaptor<op_ffi_c2php_conv> conv) {
  set_lca(conv->php_type->to_type_data());
}

void ExprNodeRecalc::recalc_ternary(VertexAdaptor<op_ternary> ternary) {
  set_lca(ternary->true_expr());
  set_lca(ternary->false_expr());
}

void ExprNodeRecalc::recalc_func_call(VertexAdaptor<op_func_call> call) {
  FunctionPtr function = call->func_id;

  if (function->return_typehint && function->return_typehint->has_argref_inside()) {
    function->return_typehint->recalc_type_data_in_context_of_call(new_type_, call);
  } else {
    set_lca(function, -1);
  }

  // TODO: is this the right place to do it?
  if (function->is_result_array2tuple) {
    new_type_->set_tuple_as_array_flag();
  }
}

void ExprNodeRecalc::recalc_arg_ref_rule(VertexAdaptor<op_trigger_recalc_arg_ref_rule> expr) {
  expr->type_hint->recalc_type_data_in_context_of_call(new_type_, expr->func_call);
}

void ExprNodeRecalc::recalc_var(VertexAdaptor<op_var> var) {
  set_lca(var->var_id);
}

void ExprNodeRecalc::recalc_push_back_return(VertexAdaptor<op_push_back_return> pb) {
  set_lca(pb->array(), &MultiKey::any_key(1));
}

void ExprNodeRecalc::recalc_index(VertexAdaptor<op_index> index) {
  bool is_const_int_index = index->has_key() && VertexUtil::get_actual_value(index->key())->type() == op_int_const;
  if (is_const_int_index) {
    long int_index = parse_int_from_string(VertexUtil::get_actual_value(index->key()).as<op_int_const>());
    MultiKey key({Key::int_key((int)int_index)});
    set_lca(index->array(), &key);
    return;
  }

  bool is_const_string_index = index->has_key() && VertexUtil::get_actual_value(index->key())->type() == op_string;
  if (is_const_string_index) {
    MultiKey key({Key::string_key(VertexUtil::get_actual_value(index->key())->get_string())});
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
  // FIXME: (extra dependenty)
  add_dependency(as_rvalue(arg));
  if (tinf::get_type(arg)->get_real_ptype() == tp_array) {
    set_lca(drop_optional(as_rvalue(arg)));
    // foreach/array_map/(array) on tuples and instances is an error
  } else if (vk::any_of_equal(tinf::get_type(arg)->ptype(), tp_tuple, tp_shape, tp_Class)) {
    set_lca(TypeData::get_type(tp_Error));
  } else {
    recalc_ptype<tp_array>();
    if (tinf::get_type(arg)->ptype() != tp_any) { // hack
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
  for (auto i : tuple->args()) {
    std::vector<Key> i_key_index{Key::int_key(index++)};
    MultiKey key(i_key_index);
    set_lca_at(&key, i);
  }
}

void ExprNodeRecalc::recalc_shape(VertexAdaptor<op_shape> shape) {
  recalc_ptype<tp_shape>();
  for (auto i : shape->args()) {
    auto double_arrow = i.as<op_double_arrow>();
    const std::string &str_index = VertexUtil::get_actual_value(double_arrow->key())->get_string();
    std::vector<Key> i_key_index{Key::string_key(str_index)};
    MultiKey key(i_key_index);
    set_lca_at(&key, double_arrow->value());

    const std::int64_t hash = string_hash(str_index.data(), str_index.size());
    TypeHintShape::register_known_key(hash, str_index);
  }
}

void ExprNodeRecalc::recalc_plus_minus(VertexAdaptor<meta_op_unary> expr) {
  set_lca(drop_optional(as_rvalue(expr->expr())));
  if (new_type()->ptype() == tp_string) {
    recalc_ptype<tp_mixed>();
  }
}

void ExprNodeRecalc::recalc_inc_dec(VertexAdaptor<meta_op_unary> expr) {
  // or false ???
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

  // FIXME: (extra dependency)
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
  if (VertexUtil::is_positive_constexpr_int(exponent)) {
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
    case op_ffi_load_call:
      recalc_func_call(expr.as<op_ffi_load_call>()->func_call());
      break;
    case op_ffi_addr:
      recalc_ffi_addr(expr.as<op_ffi_addr>());
      break;
    case op_ffi_cast:
      set_lca(expr.as<op_ffi_cast>()->php_type->to_type_data());
      break;
    case op_ffi_cdata_value_ref:
      recalc_expr(expr.as<op_ffi_cdata_value_ref>()->expr());
      break;
    case op_ffi_new:
      set_lca(expr.as<op_ffi_new>()->php_type->to_type_data());
      break;
    case op_ffi_array_get:
      set_lca(expr.as<op_ffi_array_get>()->c_elem_type->to_type_data());
      break;
    case op_trigger_recalc_arg_ref_rule:
      recalc_arg_ref_rule(expr.as<op_trigger_recalc_arg_ref_rule>());
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
    case op_lt:
    case op_le:
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

    case op_ffi_c2php_conv:
      recalc_c2php(expr.as<op_ffi_c2php_conv>());
      break;

    case op_ffi_php2c_conv:
      recalc_php2c(expr.as<op_ffi_php2c_conv>());
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

    case op_conv_drop_false:
      recalc_and_drop_false(expr.as<op_conv_drop_false>());
      break;
    case op_conv_drop_null:
      recalc_and_drop_null(expr.as<op_conv_drop_null>());
      break;

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
      set_lca(expr.as<meta_op_binary>()->lhs());
      break;

    case op_pow:
      recalc_power(expr.as<op_pow>());
      break;

    case op_clone:
      // for FFI cdata values, clone() drops the reference in &T and returns T;
      // for non-cdata types, drop_ref is ignored
      set_lca(ffi_rvalue_drop_ref(as_rvalue(expr.as<op_clone>()->expr())));
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
      break;
  }
}

bool ExprNodeRecalc::auto_edge_flag() {
  // on the very first recalc, add all dependent edges while calculating lca
  return !node_->was_recalc_finished_at_least_once();
}

ExprNodeRecalc::ExprNodeRecalc(tinf::ExprNode *node, tinf::TypeInferer *inferer)
  : NodeRecalc(node, inferer) {}

void ExprNodeRecalc::do_recalc() {
  // fprintf (stderr, "recalc expr %d %p %s\n", get_thread_id(), node_, node_->get_description().c_str());
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
      return expr.as<op_var>()->var_id->as_human_readable();

    case op_func_call: {
      auto func = expr.as<op_func_call>()->func_id;
      if (func->is_result_indexing) {
        return func->as_human_readable() + "[.]";
      }
      return func->is_constructor() ? "new " + func->class_id->as_human_readable() : func->as_human_readable();
    }

    case op_instance_prop:
      return convert_expr_to_human_readable(expr.as<op_instance_prop>()->instance()) + "->" + expr->get_string();

    case op_index: {
      std::string suff;
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
