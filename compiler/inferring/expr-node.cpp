#include "compiler/inferring/expr-node.h"

#include <regex>

#include "compiler/compiler-core.h"
#include "compiler/data/define-data.h"
#include "compiler/data/var-data.h"
#include "compiler/gentree.h"
#include "compiler/inferring/node-recalc.h"

class ExprNodeRecalc : public NodeRecalc {
private:
  template<PrimitiveType tp>
  void recalc_ptype();

  void recalc_require(VertexAdaptor<op_require> require);
  void recalc_ternary(VertexAdaptor<op_ternary> ternary);
  void apply_type_rule_func(VertexAdaptor<op_type_rule_func> func_type_rule, VertexPtr expr);
  void apply_type_rule_type(VertexAdaptor<op_type_rule> rule, VertexPtr expr);
  void apply_arg_ref(VertexAdaptor<op_arg_ref> arg, VertexPtr expr);
  void apply_index(VertexAdaptor<op_index> index, VertexPtr expr);
  void apply_type_rule(VertexPtr rule, VertexPtr expr);
  void recalc_func_call(VertexAdaptor<op_func_call> call);
  void recalc_constructor_call(VertexAdaptor<op_constructor_call> call);
  void recalc_var(VertexAdaptor<op_var> var);
  void recalc_push_back_return(VertexAdaptor<op_push_back_return> pb);
  void recalc_index(VertexAdaptor<op_index> index);
  void recalc_instance_prop(VertexAdaptor<op_instance_prop> index);
  void recalc_set(VertexAdaptor<op_set> set);
  void recalc_double_arrow(VertexAdaptor<op_double_arrow> arrow);
  void recalc_foreach_param(VertexAdaptor<op_foreach_param> param);
  void recalc_conv_array(VertexAdaptor<meta_op_unary> conv);
  void recalc_min_max(VertexAdaptor<meta_op_builtin_func> func);
  void recalc_array(VertexAdaptor<op_array> array);
  void recalc_tuple(VertexAdaptor<op_tuple> tuple);
  void recalc_plus_minus(VertexAdaptor<meta_op_unary> expr);
  void recalc_inc_dec(VertexAdaptor<meta_op_unary> expr);
  void recalc_noerr(VertexAdaptor<op_noerr> expr);
  void recalc_arithm(VertexAdaptor<meta_op_binary> expr);
  void recalc_define_val(VertexAdaptor<op_define_val> define_val);
  void recalc_power(VertexAdaptor<op_pow> expr);
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

void ExprNodeRecalc::recalc_require(VertexAdaptor<op_require> require) {
  FunctionPtr last_function = require->back()->get_func_id();
  set_lca(last_function, -1);
}

void ExprNodeRecalc::recalc_ternary(VertexAdaptor<op_ternary> ternary) {
  set_lca(ternary->true_expr());
  set_lca(ternary->false_expr());
}

void ExprNodeRecalc::apply_type_rule_func(VertexAdaptor<op_type_rule_func> func_type_rule, VertexPtr expr) {
  if (func_type_rule->str_val == "lca") {
    for (auto i : func_type_rule->args()) {
      //TODO: is it hack?
      apply_type_rule(i, expr);
    }
  } else if (func_type_rule->str_val == "OrFalse") {
    if (kphp_error (!func_type_rule->args().empty(), "OrFalse with no arguments")) {
      recalc_ptype<tp_Error>();
    } else {
      apply_type_rule(func_type_rule->args()[0], expr);
      recalc_ptype<tp_False>();
    }
  } else if (func_type_rule->str_val == "callback_call") {
    kphp_assert(func_type_rule->size() == 1);

    VertexAdaptor<op_arg_ref> arg = func_type_rule->args()[0];
    int callback_arg_id = arg->int_val;
    if (!expr || callback_arg_id < 1 || expr->type() != op_func_call || callback_arg_id > (int)expr->get_func_id()->get_params().size()) {
      kphp_error (0, "error in type rule");
      recalc_ptype<tp_Error>();
    }
    const FunctionPtr called_function = expr->get_func_id();
    kphp_assert(called_function->is_extern() && called_function->get_params()[callback_arg_id - 1]->type() == op_func_param_callback);

    VertexRange call_args = expr.as<op_func_call>()->args();
    VertexPtr callback_arg = call_args[callback_arg_id - 1];

    if (callback_arg->type() == op_func_ptr) {
      set_lca(callback_arg->get_func_id(), -1);
    } else {
      recalc_ptype<tp_Error>();
    }
  } else {
    kphp_error (0, format("unknown type_rule function [%s]", func_type_rule->str_val.c_str()));
    recalc_ptype<tp_Error>();
  }
}

void ExprNodeRecalc::apply_type_rule_type(VertexAdaptor<op_type_rule> rule, VertexPtr expr) {
  set_lca(rule->type_help);
  if (!rule->empty()) {
    switch (rule->type_help) {
      case tp_array: {
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

void ExprNodeRecalc::apply_arg_ref(VertexAdaptor<op_arg_ref> arg, VertexPtr expr) {
  int i = arg->int_val;
  if (!expr || i < 1 || expr->type() != op_func_call ||
      i > (int)expr->get_func_id()->get_params().size()) {
    kphp_error (0, "error in type rule");
    recalc_ptype<tp_Error>();
  }

  VertexRange call_args = expr.as<op_func_call>()->args();
  if (i - 1 < (int)call_args.size()) {
    set_lca(call_args[i - 1]);
  }
}

void ExprNodeRecalc::apply_index(VertexAdaptor<op_index> index, VertexPtr expr) {
  push_type();
  apply_type_rule(index->array(), expr);
  TypeData *type = pop_type();
  set_lca(type, &MultiKey::any_key(1));
  delete type;
}

void ExprNodeRecalc::apply_type_rule(VertexPtr rule, VertexPtr expr) {
  switch (rule->type()) {
    case op_type_rule_func:
      apply_type_rule_func(rule, expr);
      break;
    case op_type_rule:
      apply_type_rule_type(rule, expr);
      break;
    case op_arg_ref:
      apply_arg_ref(rule, expr);
      break;
    case op_index:
      apply_index(rule, expr);
      break;
    case op_class_type_rule:
      set_lca(rule.as<op_class_type_rule>()->class_ptr);
      break;
    default:
      kphp_error (0, "error in type rule");
      recalc_ptype<tp_Error>();
      break;
  }
}

void ExprNodeRecalc::recalc_func_call(VertexAdaptor<op_func_call> call) {
  FunctionPtr function = call->get_func_id();
  if (call->type_rule) {
    apply_type_rule(call->type_rule.as<meta_op_type_rule>()->rule(), call);
    return;
  }

  if (function->root->type_rule) {
    apply_type_rule(function->root->type_rule.as<meta_op_type_rule>()->rule(), call);
  } else {
    set_lca(function, -1);
  }
}

void ExprNodeRecalc::recalc_constructor_call(VertexAdaptor<op_constructor_call> call) {
  FunctionPtr function = call->get_func_id();
  kphp_error (function->class_id, "op_constructor_call has class_id nullptr");
  set_lca(function->class_id);
}

void ExprNodeRecalc::recalc_var(VertexAdaptor<op_var> var) {
  set_lca(var->get_var_id());
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
  set_lca(index->get_var_id());
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
    set_lca(drop_or_false(as_rvalue(arg)));
  } else if (tinf::get_type(arg)->ptype() == tp_tuple) {   // foreach/array_map/(array) на tuple'ах — ошибка
    set_lca(TypeData::get_type(tp_Error));
  } else {
    recalc_ptype<tp_array>();
    if (tinf::get_type(arg)->ptype() != tp_Unknown) { //hack
      set_lca_at(&MultiKey::any_key(1), tinf::get_type(arg)->get_real_ptype());
    }
  }
}

void ExprNodeRecalc::recalc_min_max(VertexAdaptor<meta_op_builtin_func> func) {
  VertexRange args = func->args();
  if (args.size() == 1) {
    set_lca(args[0], &MultiKey::any_key(1));
  } else {
    for (auto i : args) {
      set_lca(i);
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
  set_lca(drop_or_false(as_rvalue(expr->expr())));
  if (new_type()->ptype() == tp_string) {
    recalc_ptype<tp_var>();
  }
}

void ExprNodeRecalc::recalc_inc_dec(VertexAdaptor<meta_op_unary> expr) {
  //or false ???
  set_lca(drop_or_false(as_rvalue(expr->expr())));
}

void ExprNodeRecalc::recalc_noerr(VertexAdaptor<op_noerr> expr) {
  set_lca(as_rvalue(expr->expr()));
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
    set_lca(drop_or_false(as_rvalue(lhs)));
  }

  if (tinf::get_type(rhs)->ptype() == tp_bool) {
    recalc_ptype<tp_int>();
  } else {
    set_lca(drop_or_false(as_rvalue(rhs)));
  }

  if (new_type()->ptype() == tp_string) {
    recalc_ptype<tp_var>();
  }
}

void ExprNodeRecalc::recalc_define_val(VertexAdaptor<op_define_val> define_val) {
  //TODO: fix?
  set_lca(define_val->define_id->val);
}

void ExprNodeRecalc::recalc_power(VertexAdaptor<op_pow> expr) {
  VertexPtr base = expr->lhs();
  add_dependency(as_rvalue(base));
  VertexPtr exponent = expr->rhs();
  if (is_positive_constexpr_int(exponent)) {
    recalc_ptype<tp_int>();
    set_lca(drop_or_false(as_rvalue(base)));
  } else {
    recalc_ptype<tp_var>();
  }
}

void ExprNodeRecalc::recalc_expr(VertexPtr expr) {
  switch (expr->type()) {
    case op_move:
      recalc_expr(expr.as<op_move>()->expr());
      break;
    case op_require:
      recalc_require(expr);
      break;
    case op_ternary:
      recalc_ternary(expr);
      break;
    case op_func_call:
      recalc_func_call(expr);
      break;
    case op_constructor_call:
      recalc_constructor_call(expr);
      break;
    case op_common_type_rule:
    case op_gt_type_rule:
    case op_lt_type_rule:
    case op_eq_type_rule:
      apply_type_rule(expr.as<meta_op_type_rule>()->rule(), VertexPtr());
      break;
    case op_var:
      recalc_var(expr);
      break;
    case op_push_back_return:
      recalc_push_back_return(expr);
      break;
    case op_index:
      recalc_index(expr);
      break;
    case op_instance_prop:
      recalc_instance_prop(expr);
      break;
    case op_set:
      recalc_set(expr);
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
    case op_exit:
      recalc_ptype<tp_bool>();
      break;

    case op_conv_string:
    case op_concat:
    case op_string_build:
    case op_string:
      recalc_ptype<tp_string>();
      break;

    case op_conv_int:
    case op_conv_int_l:
    case op_int_const:
    case op_mod:
    case op_not:
    case op_or:
    case op_and:
    case op_xor:
    case op_fork:
      recalc_ptype<tp_int>();
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
      recalc_double_arrow(expr);
      break;

    case op_foreach_param:
      recalc_foreach_param(expr);
      break;

    case op_conv_array:
    case op_conv_array_l:
      recalc_conv_array(expr);
      break;

    case op_min:
    case op_max:
      recalc_min_max(expr);
      break;

    case op_array:
      recalc_array(expr);
      break;
    case op_tuple:
      recalc_tuple(expr);
      break;

    case op_conv_var:
    case op_null:
      recalc_ptype<tp_var>();
      break;

    case op_plus:
    case op_minus:
      recalc_plus_minus(expr);
      break;

    case op_prefix_inc:
    case op_prefix_dec:
    case op_postfix_inc:
    case op_postfix_dec:
      recalc_inc_dec(expr);
      break;

    case op_noerr:
      recalc_noerr(expr);
      break;

    case op_sub:
    case op_add:
    case op_mul:
    case op_shl:
    case op_shr:
      recalc_arithm(expr);
      break;

    case op_pow:
      recalc_power(expr);
      break;

    case op_define_val:
      recalc_define_val(expr);
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
      return "$" + expr.as<op_var>()->get_var_id()->name + print_type(expr);

    case op_func_call: {
      string function_name = expr.as<op_func_call>()->get_func_id()->get_human_readable_name();
      std::smatch matched;
      if (std::regex_match(function_name, matched, std::regex("(.+)( \\(inherited from .+?\\))"))) {
        return matched[1].str() + "(...)" + matched[2].str() + print_type(expr);
      }
      return function_name + "(...)" + print_type(expr);
    }
    case op_constructor_call:
      return "new " + expr->get_string() + "()";

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
  stringstream ss;
  ss << "as expression:" << "  " << get_expr_description(expr_) << "  " << "at " + get_location_text();
  return ss.str();
}

string tinf::ExprNode::get_location_text() {
  return stage::to_str(expr_->get_location());
}

const Location &tinf::ExprNode::get_location() {
  return expr_->get_location();
}
