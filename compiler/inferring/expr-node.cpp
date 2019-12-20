#include "compiler/inferring/expr-node.h"

#include <regex>
#include <sstream>

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

  void recalc_ternary(VertexAdaptor<op_ternary> ternary);
  void apply_type_rule_lca(VertexAdaptor<op_type_expr_lca> type_rule, VertexAdaptor<op_func_call> expr);
  void apply_type_rule_instance(VertexAdaptor<op_type_expr_instance> type_rule, VertexAdaptor<op_func_call> expr);
  void apply_type_rule_or_false(VertexAdaptor<op_type_expr_or_false> type_rule, VertexAdaptor<op_func_call> expr);
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
  void recalc_plus_minus(VertexAdaptor<meta_op_unary> expr);
  void recalc_inc_dec(VertexAdaptor<meta_op_unary> expr);
  void recalc_noerr(VertexAdaptor<op_noerr> expr);
  void recalc_arithm(VertexAdaptor<meta_op_binary> expr);
  void recalc_power(VertexAdaptor<op_pow> expr);
  void recalc_fork(VertexAdaptor<op_fork> fork);
  void recalc_null_coalesce(VertexAdaptor<op_null_coalesce> expr);
  void recalc_expr(VertexPtr expr);

public:
  ExprNodeRecalc(tinf::ExprNode *node, tinf::TypeInferer *inferer);
  tinf::ExprNode *get_node();
  void do_recalc();

  bool auto_edge_flag();
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

void ExprNodeRecalc::apply_type_rule_or_false(VertexAdaptor<op_type_expr_or_false> type_rule, VertexAdaptor<op_func_call> expr) {
  apply_type_rule(type_rule->expr(), expr);
  if (new_type()->ptype() != tp_void) {
    recalc_ptype<tp_False>();
  }
}

void ExprNodeRecalc::apply_type_rule_callback_call(VertexAdaptor<op_type_expr_callback_call> type_rule, VertexAdaptor<op_func_call> expr) {
  auto arg = type_rule->expr().as<op_type_expr_arg_ref>();
  int callback_arg_id = GenTree::get_id_arg_ref(arg, expr);
  if (callback_arg_id == -1) {
    kphp_error (0, "error in type rule");
    recalc_ptype<tp_Error>();
  }
  auto called_function = expr->func_id;
  kphp_assert(called_function->is_extern() && called_function->get_params()[callback_arg_id]->type() == op_func_param_callback);

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
  // ключевое слово callable, написанное в phpdoc/type declaration, никак на вывод типов не влияет
  // (влияет только на шаблонность функции по тому параметру)
}

void ExprNodeRecalc::apply_type_rule_type(VertexAdaptor<op_type_expr_type> rule, VertexAdaptor<op_func_call> expr) {
  // по идее, должно быть просто set_lca(rule->type_help), но отдельно не даём смешивать void|false
  if (rule->type_help == tp_False) {
    if (new_type()->ptype() != tp_void) {
      recalc_ptype<tp_False>();
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
        if (!klass->is_fully_static()) {
          set_lca(klass);
        } else {
          err_msg = "class passed as type-string may not be static";
        }
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
    case op_type_expr_or_false:
      apply_type_rule_or_false(rule.as<op_type_expr_or_false>(), expr);
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
  if (!is_const_int_index) {
    set_lca(index->array(), &MultiKey::any_key(1));
    return;
  }

  long int_index = parse_int_from_string(GenTree::get_actual_value(index->key()).as<op_int_const>());
  MultiKey key({Key::int_key((int)int_index)});
  set_lca(index->array(), &key);
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
    // foreach/array_map/(array) на tuple'ах и инстнсах — ошибка
  } else if (vk::any_of_equal(tinf::get_type(arg)->ptype(), tp_tuple, tp_Class)) {
    set_lca(TypeData::get_type(tp_Error));
  } else {
    recalc_ptype<tp_array>();
    if (tinf::get_type(arg)->ptype() != tp_Unknown) { //hack
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

void ExprNodeRecalc::recalc_plus_minus(VertexAdaptor<meta_op_unary> expr) {
  set_lca(drop_optional(as_rvalue(expr->expr())));
  if (new_type()->ptype() == tp_string) {
    recalc_ptype<tp_var>();
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
    recalc_ptype<tp_var>();
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
    recalc_ptype<tp_var>();
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
    case op_common_type_rule:
    case op_gt_type_rule:
    case op_lt_type_rule:
    case op_eq_type_rule:
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

    case op_conv_uint:
      recalc_ptype<tp_UInt>();
      break;

    case op_conv_long:
      recalc_ptype<tp_Long>();
      break;

    case op_conv_ulong:
      recalc_ptype<tp_ULong>();
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

    case op_conv_var:
      recalc_ptype<tp_var>();
      break;

    case op_null:
      recalc_ptype<tp_Null>();
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

    case op_alloc:
      set_lca(expr.as<op_alloc>()->allocated_class);
      break;

    default:
      recalc_ptype<tp_var>();
      break;
  }
}

bool ExprNodeRecalc::auto_edge_flag() {
  return node_->get_recalc_cnt() == 0;
}

ExprNodeRecalc::ExprNodeRecalc(tinf::ExprNode *node, tinf::TypeInferer *inferer) :
  NodeRecalc(node, inferer) {
}

tinf::ExprNode *ExprNodeRecalc::get_node() {
  return (tinf::ExprNode *)node_;
}

void ExprNodeRecalc::do_recalc() {
  tinf::ExprNode *node = get_node();
  VertexPtr expr = node->get_expr();
  //fprintf (stderr, "recalc expr %d %p %s\n", get_thread_id(), node_, node_->get_description().c_str());
  stage::set_location(expr->get_location());
  recalc_expr(expr);
}

void tinf::ExprNode::recalc(tinf::TypeInferer *inferer) {
  ExprNodeRecalc(this, inferer).run();
}

static string get_expr_description(VertexPtr expr, bool with_type_hint = true) {
  auto print_type = [&](VertexPtr type_out_of) -> string {
    return with_type_hint ? " : " + colored_type_out(tinf::get_type(type_out_of)) : "";
  };

  switch (expr->type()) {
    case op_var:
      //Вывод должен совпадать с выводом в соответсвующей ветке в tinf::VarNode::get_description, чтобы детектились и убирались дубликаты в стектрейсе
      return "$" + expr.as<op_var>()->var_id->name + print_type(expr);

    case op_func_call: {
      auto func = expr.as<op_func_call>()->func_id;
      if (func->is_constructor()) {
        return "new " + func->class_id->name + "()";
      }
      string function_name = func->get_human_readable_name();
      std::smatch matched;
      if (std::regex_match(function_name, matched, std::regex("(.+)( \\(inherited from .+?\\))"))) {
        return matched[1].str() + "(...)" + matched[2].str() + print_type(expr);
      }
      return function_name + "(...)" + print_type(expr);
    }

    case op_instance_prop:
      return "->" + expr->get_string() + print_type(expr);

    case op_index: {
      string suff;
      auto orig_expr = expr;
      while (expr->type() == op_index) {
        suff += "[.]";
        expr = expr.as<op_index>()->array();
      }
      return get_expr_description(expr, false) + suff + print_type(orig_expr);
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

string tinf::ExprNode::get_description() {
  std::stringstream ss;
  ss << "as expression:" << "  " << get_expr_description(expr_) << "  " << "at " + get_location_text();
  return ss.str();
}

string tinf::ExprNode::get_location_text() {
  return stage::to_str(expr_->get_location());
}

const Location &tinf::ExprNode::get_location() {
  return expr_->get_location();
}
