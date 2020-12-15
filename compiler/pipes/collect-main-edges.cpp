// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/collect-main-edges.h"

#include "compiler/compiler-core.h"
#include "compiler/data/define-data.h"
#include "compiler/data/src-file.h"
#include "compiler/data/var-data.h"
#include "compiler/function-pass.h"
#include "compiler/gentree.h"
#include "compiler/inferring/edge.h"
#include "compiler/inferring/ifi.h"
#include "compiler/inferring/public.h"
#include "compiler/inferring/restriction-greater.h"
#include "compiler/inferring/restriction-isset.h"
#include "compiler/inferring/restriction-less.h"
#include "compiler/inferring/restriction-non-void.h"
#include "compiler/inferring/type-node.h"
#include "compiler/phpdoc.h"
#include "compiler/utils/string-utils.h"

tinf::Node *CollectMainEdgesPass::node_from_rvalue(const RValue &rvalue) {
  if (rvalue.node == nullptr) {
    kphp_assert (rvalue.type != nullptr);
    return new tinf::TypeNode(rvalue.type, stage::get_location());
  }

  return rvalue.node;
}

void CollectMainEdgesPass::require_node(const RValue &rvalue) {
  if (rvalue.node != nullptr) {
    tinf::get_inferer()->add_node(rvalue.node);
  }
}

void CollectMainEdgesPass::create_set(const LValue &lvalue, const RValue &rvalue) {
  tinf::Edge *edge = new tinf::Edge();
  edge->from = lvalue.value;
  edge->from_at = lvalue.key;
  edge->to = node_from_rvalue(rvalue);
  tinf::get_inferer()->add_edge(edge);
  tinf::get_inferer()->add_node(edge->from);
}

template<class RestrictionT>
void CollectMainEdgesPass::create_restriction(const RValue &lhs, const RValue &rhs) {
  tinf::Node *a = node_from_rvalue(lhs);
  tinf::Node *b = node_from_rvalue(rhs);
  tinf::get_inferer()->add_node(a);
  tinf::get_inferer()->add_node(b);
  tinf::get_inferer()->add_restriction(new RestrictionT(a, b));
}

void CollectMainEdgesPass::create_less(const RValue &lhs, const RValue &rhs) {
  create_restriction<RestrictionLess>(lhs, rhs);
}

void CollectMainEdgesPass::create_greater(const RValue &lhs, const RValue &rhs) {
  create_restriction<RestrictionGreater>(lhs, rhs);
}

void CollectMainEdgesPass::create_non_void(const RValue &lhs) {
  tinf::Node *a = node_from_rvalue(lhs);
  tinf::get_inferer()->add_node(a);
  tinf::get_inferer()->add_restriction(new RestrictionNonVoid(a));
}


void CollectMainEdgesPass::create_isset_check(const RValue &rvalue) {
  tinf::Node *a = node_from_rvalue(rvalue);
  tinf::get_inferer()->add_node(a);
  tinf::get_inferer()->add_restriction(new RestrictionIsset(a));
}

RValue CollectMainEdgesPass::as_set_value(VertexPtr v) {
  if (v->type() == op_set) {
    return as_rvalue(v.as<op_set>()->rhs());
  }

  if (vk::any_of_equal(v->type(), op_prefix_inc, op_prefix_dec, op_postfix_dec, op_postfix_inc)) {
    auto unary = v.as<meta_op_unary>();
    auto location = stage::get_location();
    auto one = GenTree::create_int_const(1).set_location(location);
    auto res = VertexAdaptor<op_add>::create(unary->expr(), one).set_location(location);
    return as_rvalue(res);
  }

  if (auto binary = v.try_as<meta_op_binary>()) {
    VertexPtr res = create_vertex(OpInfo::base_op(v->type()), binary->lhs(), binary->rhs());
    res.set_location(stage::get_location());
    return as_rvalue(res);
  }

  kphp_fail();
  return RValue();
}

template<class A, class B>
void CollectMainEdgesPass::create_set(const A &a, const B &b) {
  create_set(as_lvalue(a), as_rvalue(b));
}

template<class A, class B>
void CollectMainEdgesPass::create_less(const A &a, const B &b) {
  create_less(as_rvalue(a), as_rvalue(b));
}

template<class A, class B>
void CollectMainEdgesPass::create_greater(const A &a, const B &b) {
  create_greater(as_rvalue(a), as_rvalue(b));
}

template<class A>
void CollectMainEdgesPass::require_node(const A &a) {
  require_node(as_rvalue(a));
}

template<class A>
void CollectMainEdgesPass::create_non_void(const A &a) {
  create_non_void(as_rvalue(a));
}


void CollectMainEdgesPass::add_type_rule(VertexPtr v) {
  kphp_assert(v->type() != op_function);

  switch (v->type_rule->type()) {
    case op_common_type_rule:
      create_set(v, v->type_rule);
      break;
    case op_gt_type_rule:
      create_greater(v, v->type_rule);
      break;
    case op_lt_type_rule:
      create_less(v, v->type_rule);
      break;
    case op_set_check_type_rule:
      create_set(v, v->type_rule);
      create_less(v, v->type_rule);
      break;
    default:
      __builtin_unreachable();
  }
}

void CollectMainEdgesPass::add_type_help(VertexPtr v) {
  if (v->type() != op_var) {
    return;
  }
  create_set(v, v->type_help);
}

void CollectMainEdgesPass::on_func_param_callback(VertexAdaptor<op_func_call> call, int id) {
  const FunctionPtr call_function = call->func_id;
  const VertexPtr ith_argument_of_call = call->args()[id];
  auto callback_param = call_function->get_params()[id].as<op_func_param_typed_callback>();

  FunctionPtr callback_function;
  if (auto ptr = ith_argument_of_call.try_as<op_func_ptr>()) {
    callback_function = ptr->func_id;
  }

  kphp_assert(callback_function);

  // // restriction on return type
  bool is_any = false;
  if (auto rule = callback_param->type_rule.try_as<op_common_type_rule>()) {
    if (auto son = rule->rule().try_as<op_type_expr_type>()) {
      is_any = son->type_help == tp_Any;

      if (!is_any && callback_function && callback_function->is_extern()) {
        if (auto rule_of_callback = callback_function->root->type_rule.try_as<op_common_type_rule>()) {
          if (auto son_of_callback_rule = rule_of_callback->rule().try_as<op_type_expr_type>()) {
            is_any = son_of_callback_rule->type_help == son->type_help;
          }
        }
      }
    }
  }

  if (!is_any) {
    auto fake_func_call = VertexAdaptor<op_func_call>::create(call->get_next());
    fake_func_call->type_rule = callback_param->type_rule;
    fake_func_call->func_id = call_function;
    create_less(as_rvalue(callback_function, -1), fake_func_call);
  } else {
    create_non_void(as_rvalue(callback_function, -1));
  }

  if (callback_function->is_extern()) {
    return;
  }

  VertexRange callback_args = get_function_params(callback_param);
  for (int i = 0; i < callback_args.size(); ++i) {
    auto callback_ith_arg = callback_args[i].as<op_func_param>();

    if (auto type_rule = callback_ith_arg->type_rule) {
      auto fake_func_call = VertexAdaptor<op_func_call>::create(call->get_next());
      fake_func_call->type_rule = type_rule;
      fake_func_call->func_id = call_function;

      int id_of_callbak_argument = callback_function->is_lambda() ? i + 1 : i;
      create_set(as_lvalue(callback_function, id_of_callbak_argument), fake_func_call);
    }
  }
}

void CollectMainEdgesPass::on_func_call(VertexAdaptor<op_func_call> call) {
  FunctionPtr function = call->func_id;
  VertexRange function_params = function->get_params();

  if (function->is_extern()) {
    if (function->name == "array_unshift" || function->name == "array_push") {
      VertexRange args = call->args();
      LValue val = as_lvalue(args[0]);

      auto key = new MultiKey(*val.key);
      key->push_back(Key::any_key());
      val.key = key;

      for (auto i : VertexRange(args.begin() + 1, args.end())) {
        create_set(val, i);
      }
    }

    if (function->name == "array_merge_into") {
      VertexPtr another_array_arg = call->args()[1];
      LValue dest_array_arg = as_lvalue(call->args()[0]);

      create_set(dest_array_arg, another_array_arg);
    }

    if (function->name == "array_splice" && call->args().size() == 4) {
      VertexPtr another_array_arg = call->args()[3];
      LValue dest_array_arg = as_lvalue(call->args()[0]);

      create_set(dest_array_arg, another_array_arg);
    }


    if (function->name == "wait_queue_push") {
      VertexRange args = call->args();
      LValue val = as_lvalue(args[0]);

      auto fake_func_call = VertexAdaptor<op_func_call>::create(call->get_next());
      auto ref = VertexAdaptor<op_type_expr_arg_ref>::create();
      ref->int_val = 2;
      auto rule = GenTree::create_type_help_vertex(tp_future_queue, {VertexAdaptor<op_index>::create(ref)});
      fake_func_call->type_rule = VertexAdaptor<op_common_type_rule>::create(rule);
      fake_func_call->func_id = call->func_id;

      create_set(val, fake_func_call);
    }
  }

  for (int i = 0; i < call->args().size(); ++i) {
    VertexPtr arg = call->args()[i];
    auto param = function_params[i].as<meta_op_func_param>();

    if (param->type() == op_func_param_typed_callback) {
      on_func_param_callback(call, i);
    } else {
      if (!function->is_extern()) {
        create_set(as_lvalue(function, i), arg);
      }

      if (param->var()->ref_flag) {
        create_set(arg, as_rvalue(function, i));
      }
    }
  }
}

void CollectMainEdgesPass::on_return(VertexAdaptor<op_return> v) {
  have_returns = true;
  if (v->has_expr()) {
    create_set(as_lvalue(stage::get_function(), -1), v->expr());
  } else {
    create_set(as_lvalue(stage::get_function(), -1), tp_void);
  }
}

void CollectMainEdgesPass::on_foreach(VertexAdaptor<op_foreach> foreach_op) {
  auto params = foreach_op->params();
  auto xs = params->xs();
  auto x = params->x();
  VertexAdaptor<op_var> key;
  if (params->has_key()) {
    key = params->key();
  }
  if (x->ref_flag) {
    LValue xs_tinf = as_lvalue(xs);
    create_set(xs_tinf, tp_array);
    create_set(params, x->var_id);
  } else {
    auto temp_var = params->temp_var().as<op_var>();
    create_set(temp_var->var_id, xs);
  }
  create_set(x->var_id, params);
  if (key) {
    create_set(key->var_id, tp_mixed);
  }
}

void CollectMainEdgesPass::on_list(VertexAdaptor<op_list> list) {
  for (auto cur : list->list()) {
    const auto kv = cur.as<op_list_keyval>();
    // we would like to express array[i] as rvalue multikey int_key, but we lose that
    // key type information during the edges creation (from_node[from_at] = to_node),
    // therefore we have op_index ($kv->var = $list_array[$kv->key]);
    // see the comment above build_real_multikey()
    auto new_v = VertexAdaptor<op_index>::create(list->array(), kv->key());
    new_v.set_location(stage::get_location());
    create_set(kv->var(), new_v);
  }
}

void CollectMainEdgesPass::on_throw(VertexAdaptor<op_throw> throw_op) {
  create_less(G->get_class("Exception"), throw_op->exception());
  create_less(throw_op->exception(), G->get_class("Exception"));
}

void CollectMainEdgesPass::on_try(VertexAdaptor<op_try> try_op) {
  create_set(try_op->exception(), G->get_class("Exception"));
}

void CollectMainEdgesPass::on_set_op(VertexPtr v) {
  VertexPtr lval;
  if (auto binary_op = v.try_as<meta_op_binary>()) {
    lval = binary_op->lhs();
  } else if (auto unary_op = v.try_as<meta_op_unary>()) {
    lval = unary_op->expr();
  } else {
    kphp_fail();
  }
  create_set(lval, as_set_value(v));
}
void CollectMainEdgesPass::ifi_fix(VertexPtr v) {
  is_func_id_t ifi_tp = get_ifi_id(v);
  if (ifi_tp == ifi_error) {
    return;
  }
  for (auto cur : *v) {
    if (auto var = cur.try_as<op_var>()) {
      if (var->var_id->is_constant()) {
        continue;
      }
      if (ifi_tp == ifi_unset) {
        create_set(var, tp_mixed);
      } else if (ifi_tp == ifi_isset && var->var_id->is_global_var()) {
        create_set(var, tp_Null);
      }
    }

    if ((cur->type() == op_var && ifi_tp != ifi_unset) || (ifi_tp > ifi_isset && cur->type() == op_index)) {
      tinf::Node *node = tinf::get_tinf_node(cur);
      if (node->isset_flags == 0) {
        create_isset_check(as_rvalue(node));
      }
      node->isset_flags |= ifi_tp;
    }
  }
}

void CollectMainEdgesPass::on_class(ClassPtr klass) {
  // if there is a /** @var int|false */ comment above the class field declaration, we create a type_rule
  // from that phpdoc. It makes type inferring pass takes that type into account;
  // if the inferred types mismatch, it is reported as an error
  auto add_type_rule_from_field_phpdoc = [&](vk::string_view phpdoc_str, VertexAdaptor<op_var> field_root) -> bool {
    if (auto tag_phpdoc = phpdoc_find_tag_as_string(phpdoc_str, php_doc_tag::var)) {
      auto parsed = phpdoc_parse_type_and_var_name(*tag_phpdoc, stage::get_function());
      if (!kphp_error(parsed, fmt_format("Failed to parse phpdoc of {}", field_root->var_id->get_human_readable_name()))) {
        parsed.type_expr->location = field_root->location;
        field_root->type_rule = VertexAdaptor<op_set_check_type_rule>::create(parsed.type_expr).set_location(field_root->location);
        add_type_rule(field_root);
        return true;
      }
    }
    return false;
  };

  // If there is no /** @var ... */ but class typing is mandatory,
  // we'll use the default field initializer expression to infer the class field type
  // public $a = 0; - $a is int
  // public $a = []; - $a is any[]
  auto add_type_rule_from_phpdoc_or_default_value = [&](vk::string_view phpdoc_str, VertexAdaptor<op_var> field_root, VarPtr var) {
    if (add_type_rule_from_field_phpdoc(phpdoc_str, field_root)) {
      return;
    }
    if (!G->settings().require_class_typing.get()) {
      return;
    }
    if (var->init_val) {
      create_less(var, var->init_val);
      return;
    }
    kphp_error(klass->is_lambda(),
               fmt_format("Specify @var or default value to {}", var->get_human_readable_name()));
  };

  klass->members.for_each([&](ClassMemberInstanceField &f) {
    on_var(f.var);
    add_type_rule_from_phpdoc_or_default_value(f.phpdoc_str, f.root, f.var);
  });

  klass->members.for_each([&](ClassMemberStaticField &f) {
    on_var(f.var);
    add_type_rule_from_phpdoc_or_default_value(f.phpdoc_str, f.root, f.var);
  });
}

void CollectMainEdgesPass::on_function(FunctionPtr function) {
  VertexRange params = function->get_params();
  int params_n = static_cast<int>(params.size());

  for (int i = -1; i < params_n; i++) {
    require_node(as_rvalue(function, i));
  }

  if (function->is_extern()) {
    auto ret_type = PrimitiveType::tp_mixed;
    if (auto rule = function->root->type_rule.try_as<op_common_type_rule>()) {
      if (auto type_of_rule = rule->rule().try_as<op_type_expr_type>()) {
        ret_type = type_of_rule->type_help;
      }
    }
    create_set(as_lvalue(function, -1), ret_type);

    for (int i = 0; i < params_n; i++) {
      PrimitiveType ptype = params[i]->type_help;
      if (ptype == tp_Unknown) {
        ptype = tp_Any;
      }
      //FIXME: type is const...
      create_set(as_lvalue(function, i), TypeData::get_type(ptype, tp_Any));
    }
  } else {
    for (int i = 0; i < params_n; i++) {
      //FIXME?.. just use pointer to node?..
      create_set(as_lvalue(function, i), function->param_ids[i]);
      create_set(function->param_ids[i], as_rvalue(function, i));
    }

    using InferType = FunctionData::InferHint::InferType;
    // @kphp-infer hint/check for @param/@return — is less/set on the associated tinf_node(s) function.
    for (const FunctionData::InferHint &hint : function->infer_hints) {
      switch (hint.infer_type) {
        case InferType::hint_check:
          create_set(as_lvalue(function, hint.param_i), VertexAdaptor<op_common_type_rule>::create(hint.type_expr));
          create_less(as_rvalue(function, hint.param_i), VertexAdaptor<op_lt_type_rule>::create(hint.type_expr));
          break;
        case InferType::check:
          create_less(as_rvalue(function, hint.param_i), VertexAdaptor<op_lt_type_rule>::create(hint.type_expr));
          break;
        case InferType::hint:
          create_set(as_lvalue(function, hint.param_i), VertexAdaptor<op_common_type_rule>::create(hint.type_expr));
          break;
        case InferType::cast:
          // do nothing: cast is a type_help attached in parse_and_apply_function_kphp_phpdoc()
          break;
      }
    }

    if (function->assumption_for_return) {
      create_less(as_rvalue(function, -1), function->assumption_for_return->get_type_data());
    }

    if (function->has_variadic_param) {
      auto id_of_last_param = function->get_params().size() - 1;
      RValue array_of_any{TypeData::get_type(tp_array, tp_Any)};
      create_less(as_rvalue(function, id_of_last_param), array_of_any);
    }
  }
}

void CollectMainEdgesPass::on_start() {
  if (current_function->type == FunctionData::func_class_holder) {
    on_class(current_function->class_id);
  } else {
    on_function(current_function);
  }
}

bool CollectMainEdgesPass::user_recursion(VertexPtr) {
  return current_function->is_extern();
}

VertexPtr CollectMainEdgesPass::on_enter_vertex(VertexPtr v) {
  if (current_function->is_extern()) {
    return v;
  }

  if (v->type_rule) {
    add_type_rule(v);
  }
  //FIXME: type_rule should be used indead of type_help
  if (v->type_help != tp_Unknown) {
    add_type_help(v);
  }

  switch (v->type()) {
    //FIXME: has_variadic_param
    case op_func_call:
      on_func_call(v.as<op_func_call>());
      break;
    case op_return:
      on_return(v.as<op_return>());
      break;
    case op_foreach:
      on_foreach(v.as<op_foreach>());
      break;
    case op_list:
      on_list(v.as<op_list>());
      break;
    case op_throw:
      on_throw(v.as<op_throw>());
      break;
    case op_try:
      on_try(v.as<op_try>());
      break;
    default:
      break;
  }
  if (OpInfo::rl(v->type()) == rl_set || vk::any_of_equal(v->type(), op_prefix_inc, op_postfix_inc, op_prefix_dec, op_postfix_dec)) {
    on_set_op(v);
  }

  ifi_fix(v);

  return v;
}

void CollectMainEdgesPass::on_var(VarPtr var) {
  if (var->tinf_flag) {
    return;
  }
  if (!__sync_bool_compare_and_swap(&var->tinf_flag, false, true)) {
    return;
  }
  require_node(var);
  if (var->init_val) {
    create_set(var, var->init_val);
  }

  // for all variables that are instances (local vars, params, etc.) we add a restrictions that
  // classes that are being used match the classes listed in the phpdoc
  auto assum = var->is_class_instance_var()
               ? assumption_get_for_var(var->class_id, var->name)
               : assumption_get_for_var(current_function, var->name);
  if (!assum) {
    return;
  }
  auto assum_instance = assum.try_as<AssumInstance>();

  const auto *assum_type_data = assum->get_type_data();
  if (!assum->is_primitive()) {
    create_set(var, assum_type_data);
    // $x = get_instance_arr();
    // $x = null
    // $x will have assumption `Instance[]`
    // we allow mix it with null and false only
    if (!assum_instance) {
      auto *tmp_td = assum_type_data->clone();
      tmp_td->set_or_false_flag();
      tmp_td->set_or_null_flag();
      assum_type_data = tmp_td;
    }
    create_less(var, assum_type_data);
  } else if (assum_instance && assum_instance->klass->get_namespace() == LambdaClassData::get_lambda_namespace()) {
    create_set(var, assum_type_data);
    create_less(var, assum_type_data);
  }
}

void CollectMainEdgesPass::on_finish() {
  if (current_function->is_extern()) {
    return;
  }
  if (!have_returns) {
    // hack to work well with functions which always throws
    create_set(as_lvalue(current_function, -1), tp_void);
  }
  call_on_var(current_function->local_var_ids);
  call_on_var(current_function->global_var_ids);
  call_on_var(current_function->static_var_ids);
  call_on_var(current_function->implicit_const_var_ids);
  call_on_var(current_function->explicit_const_var_ids);
  call_on_var(current_function->explicit_header_const_var_ids);
  call_on_var(current_function->param_ids);
}

template<class CollectionT>
void CollectMainEdgesPass::call_on_var(const CollectionT &collection) {
  for (const auto &el: collection) {
    on_var(el);
  }
}

// We would like to turn $a[6][$idx] = ... into a multikey (int 6, any) instead of AnyKey(2),
// but the real code contains a lot of numerical indexes which leads to a big memory usage
// increase during the type inference.
//
// This is one of the reasons why tuples are read-only.
// Writing to an index leads to any key type, but reading is OK (see recalc_index in the type inference).
/*
MultiKey *build_real_multikey (VertexPtr v) {
  MultiKey *key = new MultiKey();

  if (v->type() == op_foreach_param) {
    key->push_front(Key::any_key());
    v = v.as <op_foreach_param>()->xs();
  }
  while (v->type() == op_index) {
    if (v.as <op_index>()->has_key() && v.as <op_index>()->key()->type() == op_int_const) {
      key->push_front(Key::int_key(std::atoi(v.as <op_index>()->key()->get_string().c_str())));
    } else {
      key->push_front(Key::any_key());
    }
    v = v.as <op_index>()->array();
  }

  return key;
}
*/
